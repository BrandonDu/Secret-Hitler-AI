#pragma once
#include "BeliefState.hpp"
#include "GameState.hpp"
#include "Types.hpp"
#include <array>
#include <memory>
#include <vector>

namespace secret_hitler {

static constexpr int SH_NUM_PLAYERS = 5;

class ISMCTS_MO {
public:
  struct Node {
    Action action{};
    Node *parent{nullptr};
    std::vector<std::unique_ptr<Node>> children;
    std::vector<Action> untried; // legal actions to expand
    int visits{0};
    std::array<double, SH_NUM_PLAYERS> totalReward{}; // per-player perspective
    double policyPrior{0.0}; // policy prior from transformer for this action
    Node(const Action &a = Action(), Node *p = nullptr);
  };

  explicit ISMCTS_MO(int myPlayer, double cpuct = 1.4142135623730951);

  // Run search from root state with per-player beliefs
  Action run(const GameState &rootSt,
             const std::array<BeliefState, SH_NUM_PLAYERS> &rootBeliefs,
             std::mt19937 &rng, int iterations);

private:
  std::unique_ptr<Node> m_root;
  int m_rootPlayer;
  double m_C;

  Node *selectChildUCT(Node *node) const;
  double uctScore(const Node *child, const Node *parent) const;

  // Compute policy priors for legal actions using transformer
  std::vector<double>
  computePolicyPriors(const GameState &state,
                      const std::array<BeliefState, SH_NUM_PLAYERS> &beliefs,
                      const std::vector<Action> &legalActions, int actor,
                      std::mt19937 &rng) const;

  // rollout that re-determinizes from the ACTOR'S belief at each decision
  int playoutMO(GameState &sim,
                std::array<BeliefState, SH_NUM_PLAYERS> &beliefs,
                std::mt19937 &rng) const;

  bool playerIsLiberalInRoot(int p, const GameState &rootSt) const;
};

} // namespace secret_hitler