#include "ISMCTS_MO.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>

#include "BeliefUpdate.hpp"
#include "FeatureMode.hpp"
#include "Features.hpp"
#include "LearnModels.hpp"
#include "NeuralPolicy.hpp"
#include "Search.hpp"

namespace secret_hitler {

ISMCTS_MO::Node::Node(const Action &a, Node *p)
    : action(a), parent(p), visits(0), policyPrior(0.0) {
  totalReward.fill(0.0);
}

ISMCTS_MO::ISMCTS_MO(int myPlayer, double cpuct)
    : m_rootPlayer(myPlayer), m_C(cpuct) {
  m_root = std::make_unique<Node>(Action(), nullptr);
}

Action
ISMCTS_MO::run(const GameState &rootSt,
               const std::array<BeliefState, SH_NUM_PLAYERS> &rootBeliefs,
               std::mt19937 &rng, int iterations) {
  m_root = std::make_unique<Node>(Action(), nullptr);
  m_root->untried = rootSt.getLegalActions();

  for (int it = 0; it < iterations; ++it) {
    GameState state = rootSt;
    auto beliefs = rootBeliefs;

    Node *node = m_root.get();
    while (node->untried.empty() && !node->children.empty()) {
      Node *next = selectChildUCT(node);
      GameState before = state;
      state.apply(next->action, rng);
      for (int p = 0; p < SH_NUM_PLAYERS; ++p)
        updateBelief(beliefs[p], before, next->action);
      node = next;
    }

    if (!node->untried.empty()) {
      // Compute policy priors for untried actions
      int actor = node->untried.front().actor;
      auto priors =
          computePolicyPriors(state, beliefs, node->untried, actor, rng);

      // Select action based on policy priors (proportional sampling)
      double totalPrior = 0.0;
      for (double p : priors)
        totalPrior += p;

      // If no valid priors, use uniform
      if (totalPrior <= 0.0) {
        for (double &p : priors)
          p = 1.0;
        totalPrior = priors.size();
      }

      // Sample action proportional to policy prior
      std::uniform_real_distribution<double> dist(0.0, totalPrior);
      double sample = dist(rng);
      double cumsum = 0.0;
      size_t selectedIdx = 0;
      for (size_t i = 0; i < priors.size(); ++i) {
        cumsum += priors[i];
        if (sample <= cumsum) {
          selectedIdx = i;
          break;
        }
      }

      Action a = node->untried[selectedIdx];
      // Priors are already normalized from computePolicyPriors
      double prior = priors[selectedIdx];

      // Remove selected action from untried
      node->untried.erase(node->untried.begin() + selectedIdx);

      GameState before = state;
      state.apply(a, rng);
      for (int p = 0; p < SH_NUM_PLAYERS; ++p)
        updateBelief(beliefs[p], before, a);

      auto child = std::make_unique<Node>(a, node);
      child->policyPrior = prior;
      child->untried = state.getLegalActions();
      node->children.push_back(std::move(child));
      node = node->children.back().get();
    }

    GameState sim = state;
    int winner = playoutMO(sim, beliefs, rng);

    for (Node *n = node; n != nullptr; n = n->parent) {
      n->visits += 1;
      for (int p = 0; p < SH_NUM_PLAYERS; ++p) {
        double sign = playerIsLiberalInRoot(p, rootSt) ? 1.0 : -1.0;
        n->totalReward[p] += winner * sign;
      }
    }
  }

  Node *best = nullptr;
  int bestVisits = -1;
  for (auto &ch : m_root->children) {
    if (ch->visits > bestVisits) {
      bestVisits = ch->visits;
      best = ch.get();
    }
  }
  return best ? best->action : Action();
}

ISMCTS_MO::Node *ISMCTS_MO::selectChildUCT(Node *node) const {
  return std::max_element(node->children.begin(), node->children.end(),
                          [&](const auto &a, const auto &b) {
                            return uctScore(a.get(), node) <
                                   uctScore(b.get(), node);
                          })
      ->get();
}

double ISMCTS_MO::uctScore(const Node *child, const Node *parent) const {
  if (child->visits == 0)
    return std::numeric_limits<double>::infinity();
  double Q = child->totalReward[m_rootPlayer] / child->visits;

  // PUCT formula: U = c_puct * P * sqrt(sum_N) / (1 + N)
  // where P is policy prior, sum_N is sum of sibling visits
  double sumVisits = 0.0;
  for (const auto &sibling : parent->children)
    sumVisits += sibling->visits;

  double U =
      m_C * child->policyPrior * std::sqrt(sumVisits) / (1.0 + child->visits);
  return Q + U;
}

bool ISMCTS_MO::playerIsLiberalInRoot(int p, const GameState &rootSt) const {
  return rootSt.getRoles()[p] == Role::Liberal;
}

std::vector<double> ISMCTS_MO::computePolicyPriors(
    const GameState &state,
    const std::array<BeliefState, SH_NUM_PLAYERS> &beliefs,
    const std::vector<Action> &legalActions, int actor,
    std::mt19937 &rng) const {
  std::vector<double> priors(legalActions.size(), 0.0);

  if (legalActions.empty())
    return priors;

  // Determinize from actor's perspective using the search RNG for proper
  // variance
  GameState det = determinize(state, beliefs[actor], actor, rng);

  FeatureMode mode = getFeatureMode();

  // Check action type
  if (legalActions.front().type == ActionType::Vote) {
    // For voting, use transformer to get P(vote yes)
    auto phi = extractVotingFeatures(det, beliefs[actor], actor);
    double pYes;

    if (mode == FeatureMode::ManualOnly) {
      pYes = computeVoteYesProb(phi, state.getRoles()[actor]);
    } else if (mode == FeatureMode::TransformerOnly) {
      pYes = transformerVoteProb(phi, state.getRoles()[actor]);
    } else // Both
    {
      double p1 = computeVoteYesProb(phi, state.getRoles()[actor]);
      double p2 = transformerVoteProb(phi, state.getRoles()[actor]);
      pYes = 0.5 * (p1 + p2);
    }

    // Assign priors: P(yes) to yes actions, P(no) to no actions
    for (size_t i = 0; i < legalActions.size(); ++i) {
      if (legalActions[i].voteYes)
        priors[i] = pYes;
      else
        priors[i] = 1.0 - pYes;
    }
  } else if (legalActions.front().type == ActionType::Enact) {
    // For enactment, use transformer to get P(enact fascist)
    auto phi = extractEnactFeatures(det);
    double pFascist;

    if (mode == FeatureMode::ManualOnly) {
      pFascist = computeEnactFascistProb(phi, state.getRoles()[actor]);
    } else if (mode == FeatureMode::TransformerOnly) {
      pFascist = transformerEnactProb(phi, state.getRoles()[actor]);
    } else // Both
    {
      double p1 = computeEnactFascistProb(phi, state.getRoles()[actor]);
      double p2 = transformerEnactProb(phi, state.getRoles()[actor]);
      pFascist = 0.5 * (p1 + p2);
    }

    // Assign priors based on whether action enacts fascist or liberal
    for (size_t i = 0; i < legalActions.size(); ++i) {
      Policy chosen = state.getDrawBuf()[legalActions[i].index];
      if (chosen == Policy::Fascist)
        priors[i] = pFascist;
      else
        priors[i] = 1.0 - pFascist;
    }
  } else {
    // For other action types (Nominate, DrawDiscard, Execute, Veto), use
    // uniform prior
    double uniformPrior = 1.0 / legalActions.size();
    for (size_t i = 0; i < legalActions.size(); ++i)
      priors[i] = uniformPrior;
  }

  // Normalize to ensure they sum to 1
  double sum = 0.0;
  for (double p : priors)
    sum += p;
  if (sum > 0.0) {
    for (double &p : priors)
      p /= sum;
  } else {
    // Fallback to uniform if all priors are zero
    double uniformPrior = 1.0 / legalActions.size();
    for (double &p : priors)
      p = uniformPrior;
  }

  return priors;
}

int ISMCTS_MO::playoutMO(GameState &sim,
                         std::array<BeliefState, SH_NUM_PLAYERS> &beliefs,
                         std::mt19937 &rng) const {
  while (!sim.isTerminal()) {
    auto leg = sim.getLegalActions();
    if (leg.empty())
      break;

    if (!leg.empty() && leg.front().type == ActionType::Vote) {
      for (int voter = 0; voter < SH_NUM_PLAYERS; ++voter) {
        if (!sim.isAlive()[voter])
          continue;

        GameState det_v = determinize(sim, beliefs[voter], voter, rng);
        auto phi = extractVotingFeatures(det_v, beliefs[voter], voter);

        FeatureMode mode = getFeatureMode();
        double pY;
        if (mode == FeatureMode::ManualOnly) {
          pY = computeVoteYesProb(phi, sim.getRoles()[voter]);
        } else if (mode == FeatureMode::TransformerOnly) {
          pY = transformerVoteProb(phi, sim.getRoles()[voter]);
        } else {
          double p1 = computeVoteYesProb(phi, sim.getRoles()[voter]);
          double p2 = transformerVoteProb(phi, sim.getRoles()[voter]);
          pY = 0.5 * (p1 + p2);
        }

        bool yes = std::bernoulli_distribution(pY)(rng);

        Action voteAction{ActionType::Vote, voter, -1, yes, -1};
        GameState before = sim;
        sim.apply(voteAction, rng);
        for (int p = 0; p < SH_NUM_PLAYERS; ++p)
          updateBelief(beliefs[p], before, voteAction);
      }
      continue;
    }

    int actor = leg.front().actor;
    GameState det = determinize(sim, beliefs[actor], actor, rng);
    auto detActs = det.getLegalActions();
    Action chosen;

    if (!detActs.empty() && detActs.front().type == ActionType::Enact) {
      int enactor = detActs.front().actor;
      auto phi = extractEnactFeatures(det);

      FeatureMode mode = getFeatureMode();
      double pF;
      if (mode == FeatureMode::ManualOnly) {
        pF = computeEnactFascistProb(phi, sim.getRoles()[enactor]);
      } else if (mode == FeatureMode::TransformerOnly) {
        pF = transformerEnactProb(phi, sim.getRoles()[enactor]);
      } else {
        double p1 = computeEnactFascistProb(phi, sim.getRoles()[enactor]);
        double p2 = transformerEnactProb(phi, sim.getRoles()[enactor]);
        pF = 0.5 * (p1 + p2);
      }

      bool chooseF = std::bernoulli_distribution(pF)(rng);
      bool found = false;
      for (auto &ac : leg) {
        Policy c = sim.getDrawBuf()[ac.index];
        if ((c == Policy::Fascist) == chooseF) {
          chosen = ac;
          found = true;
          break;
        }
      }
      if (!found)
        chosen = leg.front();
    } else if (!detActs.empty() &&
               detActs.front().type == ActionType::DrawDiscard) {
      bool discarded = false;
      if (sim.getRoles()[actor] == Role::Liberal) {
        for (auto &ac : leg) {
          if (sim.getDrawBuf()[ac.index] == Policy::Fascist) {
            chosen = ac;
            discarded = true;
            break;
          }
        }
      } else {
        for (auto &ac : leg) {
          if (sim.getDrawBuf()[ac.index] == Policy::Liberal) {
            chosen = ac;
            discarded = true;
            break;
          }
        }
      }
      if (!discarded)
        chosen = leg.front();
    } else {
      std::uniform_int_distribution<int> d(0, (int)leg.size() - 1);
      chosen = leg[d(rng)];
    }

    GameState before = sim;
    sim.apply(chosen, rng);
    for (int p = 0; p < SH_NUM_PLAYERS; ++p)
      updateBelief(beliefs[p], before, chosen);
  }
  return sim.getWinner();
}

} // namespace secret_hitler
