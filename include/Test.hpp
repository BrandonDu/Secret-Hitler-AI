#pragma once

#include "Types.hpp"
#include "GameState.hpp"
#include "ISMCTS.hpp"
#include "Search.hpp"
#include "LearnModels.hpp"
#include "BeliefUpdate.hpp"
#include "Features.hpp"
#include "ProgressBar.hpp"
#include "Bots.hpp"

#include <string>
#include <vector>
#include <array>
#include <random>
#include <functional>
#include <chrono>
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>

namespace secret_hitler
{

    inline std::string to_string(ActionType action)
    {
        switch (action)
        {
        case ActionType::Nominate:
            return "Nominate";
        case ActionType::Vote:
            return "Vote";
        case ActionType::DrawDiscard:
            return "DrawDiscard";
        case ActionType::Enact:
            return "Enact";
        case ActionType::Veto:
            return "Veto";
        case ActionType::Execute:
            return "Execute";
        default:
            return "Unknown";
        }
    }

    inline std::string to_string(Role r)
    {
        switch (r)
        {
        case Role::Liberal:
            return "Liberal";
        case Role::Hitler:
            return "Hitler";
        case Role::Fascist:
            return "Fascist";
        }
        return "Unknown";
    }

    inline std::string to_string(Policy p)
    {
        return p == Policy::Liberal ? "Liberal" : "Fascist";
    }

    struct MatchResult
    {
        int mctsWins = 0;
        int oppWins = 0;
    };

    template <typename Opponent>    
    MatchResult play(int numGames, int mctsIters, const char *label)
    {
        MatchResult globalResult;
        constexpr int BAR_WIDTH = 50;
        auto startTime = std::chrono::steady_clock::now();

        int numThreads = std::max(1u, std::thread::hardware_concurrency());
        std::atomic<int> completed{0};
        std::mutex resultMutex;
        std::vector<std::thread> workers;

        // Each thread plays a subset of games independently
        for (int t = 0; t < numThreads; ++t)
        {
            workers.emplace_back([&, t]()
                                 {
            std::mt19937 rng(std::random_device{}() + t * 7919);
            MatchResult localResult;

            for (int g = t; g < numGames; g += numThreads)
            {
                GameState gs(rng);
                auto roles = gs.getRoles();

                std::array<int, 5> seats = {0, 1, 2, 3, 4};
                std::shuffle(seats.begin(), seats.end(), rng);
                std::vector<int> mctsSeats(seats.begin(), seats.begin() + 3);
                std::vector<int> oppSeats(seats.begin() + 3, seats.end());

                std::vector<ISMCTS> mctsBots;
                std::vector<BeliefState> beliefs;
                mctsBots.reserve(3);
                beliefs.reserve(3);
                for (int s : mctsSeats)
                {
                    mctsBots.emplace_back(s);
                    beliefs.emplace_back();
                    initBeliefs(gs, beliefs.back(), s);
                }

                using DecideFn = std::function<Action(GameState &, std::mt19937 &)>;
                std::array<DecideFn, 5> decide;
                for (int i = 0; i < 5; ++i)
                {
                    auto it = std::find(mctsSeats.begin(), mctsSeats.end(), i);
                    if (it != mctsSeats.end())
                    {
                        int idx = std::distance(mctsSeats.begin(), it);
                        decide[i] = [idx, &mctsBots, &beliefs, mctsIters](GameState &st, std::mt19937 &r)
                        {
                            return mctsBots[idx].run(st, beliefs[idx], r, mctsIters);
                        };
                    }
                    else
                    {
                        Opponent bot(i);
                        decide[i] = [bot](GameState &st, std::mt19937 &) mutable
                        {
                            return bot.act(st);
                        };
                    }
                }

                while (!gs.isTerminal())
                {
                    auto legal = gs.getLegalActions();
                    int actor = legal.front().actor;
                    Action a = decide[actor](gs, rng);
                    if (legal.front().type == ActionType::Vote ||
                        legal.front().type == ActionType::Enact)
                    {
                        for (auto &b : beliefs)
                            updateBelief(b, gs, a);
                    }
                    gs.apply(a, rng);
                }

                int winner = gs.getWinner();
                for (int s : mctsSeats)
                {
                    bool isLib = (roles[s] == Role::Liberal);
                    if ((winner > 0 && isLib) || (winner < 0 && !isLib))
                        localResult.mctsWins++;
                }
                for (int s : oppSeats)
                {
                    bool isLib = (roles[s] == Role::Liberal);
                    if ((winner > 0 && isLib) || (winner < 0 && !isLib))
                        localResult.oppWins++;
                }

                int done = ++completed;
                if (done % 5 == 0 || done == numGames)
                    printProgressBar(label, done, numGames, BAR_WIDTH, startTime);
            }

            std::lock_guard<std::mutex> lock(resultMutex);
            globalResult.mctsWins += localResult.mctsWins;
            globalResult.oppWins += localResult.oppWins; });
        }

        for (auto &th : workers)
            th.join();

        std::cout << "\n";
        globalResult.mctsWins /= 3;
        globalResult.oppWins /= 2;
        return globalResult;
    }

    void traceOneGame(int mctsIters = 1000);

    void trainTest(const int numRounds = 5, const int numGamesPerRound = 500, const int numEpochs = 5, const int numTestingGames = 1000);
}
