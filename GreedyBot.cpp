#include "Types.hpp"
#include "GameState.hpp"
#include "ProgressBar.hpp"
#include <random>
#include <chrono>
#include <vector>
#include <array>
#include <string>

namespace secret_hitler {
    class GreedyBot {
    private:
        int idx;
    public:
        GreedyBot(int index) {
            idx = index;
        }

        Action act(GameState &gs) {
            auto acts = gs.getLegalActions();
            auto drawBuf = gs.getDrawBuf();

            ActionType act_type = acts[0].type;
            std::array<Role, 5> roles = {Role::Liberal, Role::Liberal, Role::Liberal, Role::Hitler, Role::Fascist};
            auto role = roles[idx];

            if (act_type == ActionType::Enact) {
                if (role == Role::Liberal) {
                    for (auto &ac: acts)
                        if (drawBuf[ac.index] == Policy::Liberal) {
                            return ac;
                        }
                    return acts[0];
                } else {
                    for (auto &ac: acts)
                        if (drawBuf[ac.index] == Policy::Fascist) {
                            return ac;
                        }
                    return acts[0];
                }
            } else if (act_type == ActionType::DrawDiscard) {
                if (role == Role::Liberal) {
                    for (auto &ac: acts)
                        if (drawBuf[ac.index] == Policy::Fascist) {
                            return ac;
                        }
                    return acts[0];
                } else {
                    for (auto &ac: acts)
                        if (drawBuf[ac.index] == Policy::Liberal) {
                            return ac;
                        }
                    return acts[0];
                }
            }
            else {
                std::mt19937 rng(std::random_device{}());
                std::uniform_int_distribution<int> ud(0, (int)acts.size() - 1);
                return acts[ud(rng)];
            }
        }
    };
    std::string to_string(ActionType action)
    {
        switch (action)
        {
            case ActionType::Nominate:    return "Nominate";
            case ActionType::Vote:        return "Vote";
            case ActionType::DrawDiscard: return "DrawDiscard";
            case ActionType::Enact:       return "Enact";
            case ActionType::Veto:        return "Veto";
            case ActionType::Execute:     return "Execute";
            default:                      return "Unknown";
        }
    }

    int testGreedy(int numGames) {
        std::mt19937 rng(std::random_device{}());
        GameState gs(rng);
        std::array<ISMCTS, 3> ISMCTS_bots = {
                ISMCTS(0), ISMCTS(1),
                ISMCTS(2)
        };
        std::array<GreedyBot, 2> greedy_bots = {
                GreedyBot(3),
                GreedyBot(4)
        };
        std::array<BeliefState,3> beliefs;
        for(int i = 0; i < 3; ++i) {
            initBeliefs(gs, beliefs[i], i);
        }
        constexpr int BAR_WIDTH = 50;
        auto startTime = std::chrono::steady_clock::now();
        int liberalWins =  0;
        for (int i = 0; i < numGames; ++i) {
            while (!gs.isTerminal()) {
                auto leg = gs.getLegalActions();
                if (leg.size() == 0) {
                    std::cout << "fucked up" << std::endl;
                    exit(1);
                }
                int actor = leg.front().actor;
                Action a;
                if (actor <= 2) {
                    a = ISMCTS_bots[actor].run(gs, beliefs[actor], rng, 1000);
                }
                else {
                    a = greedy_bots[actor - 3].act(gs);
                }
                for (int i = 0; i < 3; ++i) {
                    if (leg.front().type == ActionType::Vote || leg.front().type == ActionType::Enact)
                        updateBelief(beliefs[i], gs, a);
                }
                gs.apply(a, rng);
                std::cout << to_string(a.type) << " " << a.actor << " " << a.target << " " << a.voteYes << " " << a.index << std::endl;

            }
            if (gs.getWinner() > 0)
                ++liberalWins;
            std::cout << "Fascist: " << gs.getEnactedFascist() << "Liberal: " << gs.getEnactedLiberal() << std::endl;
            printProgressBar("Testing Greedy",
                             i + 1,
                             numGames,
                             BAR_WIDTH,
                             startTime);
        }
        return liberalWins;
    }
}