#include "ISMCTS.hpp"
#include "Search.hpp"

#include "Types.hpp"
#include "GameState.hpp"
#include "BeliefState.hpp"
#include "Features.hpp"
#include "BeliefUpdate.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <limits>

namespace secret_hitler
{

    ISMCTS::Node::Node(const Action &a, Node *p)
        : action(a), parent(p)
    {
    }

    ISMCTS::ISMCTS(int me, double c)
        : m_myPlayer(me), m_C(c)
    {
        m_root = std::make_unique<Node>(Action(), nullptr);
    }

    double ISMCTS::uctScore(const Node *n) const
    {
        if (n->visits == 0)
        {
            return std::numeric_limits<double>::infinity();
        }
        double exploitation = n->wins / n->visits;
        double exploration = m_C * std::sqrt(std::log(n->parent->visits) / n->visits);
        return exploitation + exploration;
    }

    Action ISMCTS::run(const GameState &rootSt, const BeliefState &rootB, std::mt19937 &rng, int iterations)
    {
        m_root = std::make_unique<Node>(Action(), nullptr);

        m_root->untried = rootSt.getLegalActions();

        for (int it = 0; it < iterations; ++it)
        {
            GameState det = determinize(rootSt, rootB, m_myPlayer, rng);

            Node *node = m_root.get();
            GameState state = det;
            while (node->untried.empty() && !node->children.empty())
            {
                node = std::max_element(node->children.begin(), node->children.end(),
                                        [&](auto &a, auto &b)
                                        {
                                            return uctScore(a.get()) < uctScore(b.get());
                                        })
                           ->get();
                state.apply(node->action, rng);
            }
            if (!node->untried.empty())
            {
                Action a = node->untried.back();
                node->untried.pop_back();
                state.apply(a, rng);
                auto child = std::make_unique<Node>(a, node);
                child->untried = state.getLegalActions();
                node->children.push_back(std::move(child));
                node = node->children.back().get();
            }

            GameState sim = state;
            BeliefState simB = rootB;
            int result;
            auto roles = sim.getRoles();

            while (!sim.isTerminal())
            {
                auto acts = sim.getLegalActions();
                if (acts[0].type == ActionType::Vote)
                {
                    for (int voter = 0; voter < 5; ++voter)
                    {
                        if (!sim.isAlive()[voter])
                            continue;
                        auto phi = extractVotingFeatures(sim, simB, voter);
                        double pYes = computeVoteYesProb(phi, roles[voter]);
                        bool vote = std::bernoulli_distribution(pYes)(rng);
                        sim.apply(Action{ActionType::Vote, voter, -1, vote, -1}, rng);
                    }
                }
                else if (acts[0].type == ActionType::Enact)
                {

                    std::vector<Action> enactActs;
                    for (auto &ac : acts)
                        if (ac.type == ActionType::Enact)
                            enactActs.push_back(ac);
                    auto actor = acts[0].actor;
                    if (roles[actor] == Role::Liberal) {
                        for (auto &ac : enactActs)
                            if (sim.getDrawBuf()[ac.index] == Policy::Liberal) {
                                sim.apply(ac, rng);
                                continue;   
                            }
                
                        sim.apply(enactActs[0], rng);
                        continue;
                    }
                    
                    auto phi = extractEnactFeatures(sim, actor);
                    double pF = computeEnactFascistProb(phi, roles[actor]);

                    bool chooseF = std::bernoulli_distribution(pF)(rng);

                    for (auto &ac : enactActs)
                    {
                        Policy c = sim.getDrawBuf()[ac.index];
                        if ((c == Policy::Fascist) == chooseF)
                        {
                            sim.apply(ac, rng);
                            break;
                        }
                    }
                }
                else
                {
                    std::uniform_int_distribution<int> d(0, (int)acts.size() - 1);
                    sim.apply(acts[d(rng)], rng);
                }
            }
            result = sim.getWinner();
            double perspective = (roles[m_myPlayer] == Role::Liberal ? +1.0 : -1.0);
            for (Node *n = node; n != nullptr; n = n->parent)
            {
                n->visits++;
                n->wins += result * perspective;
            }
        }

        Node *best = nullptr;
        int bestVisits = -1;
        for (auto &ch : m_root->children)
        {
            if (ch->visits > bestVisits)
            {
                bestVisits = ch->visits;
                best = ch.get();
            }
        }
        return best ? best->action : Action();
    }

}
