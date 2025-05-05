#pragma once
#include <memory>
#include <vector>
#include <random>

#include <Types.hpp>

namespace secret_hitler
{

    class GameState;
    struct BeliefState;

    class ISMCTS
    {
    private:
        struct Node
        {
            int visits = 0;
            double wins = 0.0;
            Action action;
            Node *parent = nullptr;
            std::vector<std::unique_ptr<Node>> children;
            std::vector<Action> untried;

            Node(const Action &a, Node *p);
        };

        int m_myPlayer;
        double m_C;
        std::unique_ptr<Node> m_root;

        double uctScore(const Node *n) const;

    public:
        explicit ISMCTS(int me, double c = 1.414);

        ~ISMCTS() = default;

        Action run(const GameState &rootSt,
                   const BeliefState &rootB,
                   std::mt19937 &rng,
                   int iterations);
    };

}
