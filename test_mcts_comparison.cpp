#include <iostream>
#include <random>
#include <vector>
#include <array>
#include <algorithm>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>

#include "GameState.hpp"
#include "ISMCTS.hpp"
#include "ISMCTS_MO.hpp"
#include "BeliefUpdate.hpp"
#include "Search.hpp"
#include "ProgressBar.hpp"
#include "FeatureMode.hpp"
#include "NeuralPolicy.hpp"

using namespace secret_hitler;

struct MatchResult {
    int ismcts_wins = 0;
    int ismcts_mo_wins = 0;
    int draws = 0;
};

MatchResult playMatch(int numGames, int iterations, bool useTransformer) {
    MatchResult globalResult;
    std::atomic<int> completed{0};
    std::mutex resultMutex;
    
    constexpr int BAR_WIDTH = 50;
    auto startTime = std::chrono::steady_clock::now();
    
    int numThreads = std::max(1u, std::thread::hardware_concurrency());
    std::vector<std::thread> workers;
    
    for (int t = 0; t < numThreads; ++t) {
        workers.emplace_back([&, t, iterations, useTransformer]() {
            std::mt19937 rng(std::random_device{}() + t * 7919);
            MatchResult localResult;
            
            for (int g = t; g < numGames; g += numThreads) {
                GameState gs(rng);
                auto roles = gs.getRoles();
                
                // Split players: first 2 use ISMCTS, last 2 use ISMCTS_MO, one random
                std::array<int, 5> seats = {0, 1, 2, 3, 4};
                std::shuffle(seats.begin(), seats.end(), rng);
                
                std::vector<int> ismctsSeats = {seats[0], seats[1]};
                std::vector<int> ismcts_moSeats = {seats[2], seats[3]};
                int neutralSeat = seats[4];
                
                // Initialize ISMCTS bots
                std::vector<ISMCTS> ismctsBots;
                std::vector<BeliefState> ismctsBeliefs;
                for (int s : ismctsSeats) {
                    ismctsBots.emplace_back(s);
                    ismctsBeliefs.emplace_back();
                    initBeliefs(gs, ismctsBeliefs.back(), s);
                }
                
                // Initialize ISMCTS_MO bots
                std::array<BeliefState, SH_NUM_PLAYERS> moBeliefs;
                for (int i = 0; i < SH_NUM_PLAYERS; ++i) {
                    initBeliefs(gs, moBeliefs[i], i);
                }
                std::vector<ISMCTS_MO> ismcts_moBots;
                for (int s : ismcts_moSeats) {
                    ismcts_moBots.emplace_back(s);
                }
                
                while (!gs.isTerminal()) {
                    auto legal = gs.getLegalActions();
                    if (legal.empty()) break;
                    
                    int actor = legal.front().actor;
                    Action a;
                    
                    // Determine which bot to use
                    auto it1 = std::find(ismctsSeats.begin(), ismctsSeats.end(), actor);
                    auto it2 = std::find(ismcts_moSeats.begin(), ismcts_moSeats.end(), actor);
                    
                    if (it1 != ismctsSeats.end()) {
                        // Use original ISMCTS
                        int idx = std::distance(ismctsSeats.begin(), it1);
                        a = ismctsBots[idx].run(gs, ismctsBeliefs[idx], rng, iterations);
                    } else if (it2 != ismcts_moSeats.end()) {
                        // Use ISMCTS_MO
                        int idx = std::distance(ismcts_moSeats.begin(), it2);
                        a = ismcts_moBots[idx].run(gs, moBeliefs, rng, iterations);
                    } else {
                        // Neutral player - use random
                        std::uniform_int_distribution<int> dist(0, legal.size() - 1);
                        a = legal[dist(rng)];
                    }
                    
                    // Update beliefs
                    GameState before = gs;
                    for (auto &b : ismctsBeliefs)
                        updateBelief(b, before, a);
                    for (int p = 0; p < SH_NUM_PLAYERS; ++p)
                        updateBelief(moBeliefs[p], before, a);
                    
                    gs.apply(a, rng);
                }
                
                int winner = gs.getWinner();
                
                // Count wins for each team
                int ismcts_team_score = 0;
                int ismcts_mo_team_score = 0;
                
                for (int s : ismctsSeats) {
                    bool isLib = (roles[s] == Role::Liberal);
                    if ((winner > 0 && isLib) || (winner < 0 && !isLib))
                        ismcts_team_score++;
                }
                
                for (int s : ismcts_moSeats) {
                    bool isLib = (roles[s] == Role::Liberal);
                    if ((winner > 0 && isLib) || (winner < 0 && !isLib))
                        ismcts_mo_team_score++;
                }
                
                if (ismcts_team_score > ismcts_mo_team_score)
                    localResult.ismcts_wins++;
                else if (ismcts_mo_team_score > ismcts_team_score)
                    localResult.ismcts_mo_wins++;
                else
                    localResult.draws++;
                
                int done = ++completed;
                if (done % 5 == 0 || done == numGames)
                    printProgressBar("ISMCTS vs ISMCTS_MO", done, numGames, BAR_WIDTH, startTime);
            }
            
            std::lock_guard<std::mutex> lock(resultMutex);
            globalResult.ismcts_wins += localResult.ismcts_wins;
            globalResult.ismcts_mo_wins += localResult.ismcts_mo_wins;
            globalResult.draws += localResult.draws;
        });
    }
    
    for (auto &th : workers)
        th.join();
    
    std::cout << "\n";
    return globalResult;
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <num_games> <iterations> <transformer_mode>\n";
        std::cerr << "  transformer_mode: manual|transformer|both\n";
        return 1;
    }
    
    int numGames = std::stoi(argv[1]);
    int iterations = std::stoi(argv[2]);
    std::string modeStr = argv[3];
    
    FeatureMode featMode = FeatureMode::ManualOnly;
    if (modeStr == "transformer")
        featMode = FeatureMode::TransformerOnly;
    else if (modeStr == "both")
        featMode = FeatureMode::Both;
    
    setFeatureMode(featMode);
    
    constexpr int INPUT_DIM = 32;
    if (featMode != FeatureMode::ManualOnly) {
        try {
            initTransformerModel("src/sh_transformer.pt", INPUT_DIM);
        } catch (const std::exception &e) {
            std::cerr << "Failed to load transformer: " << e.what() << "\n";
            return 1;
        }
    }
    
    std::cout << "=== ISMCTS (Original) vs ISMCTS_MO (Multi-Observer + Transformer) ===\n";
    std::cout << "Games: " << numGames << ", Iterations: " << iterations << ", Mode: " << modeStr << "\n\n";
    
    MatchResult result = playMatch(numGames, iterations, featMode != FeatureMode::ManualOnly);
    
    int total = result.ismcts_wins + result.ismcts_mo_wins + result.draws;
    double ismcts_pct = 100.0 * result.ismcts_wins / total;
    double ismcts_mo_pct = 100.0 * result.ismcts_mo_wins / total;
    double draw_pct = 100.0 * result.draws / total;
    
    std::cout << "\nResults:\n";
    std::cout << "ISMCTS (Original): " << result.ismcts_wins << " wins (" << ismcts_pct << "%)\n";
    std::cout << "ISMCTS_MO: " << result.ismcts_mo_wins << " wins (" << ismcts_mo_pct << "%)\n";
    std::cout << "Draws: " << result.draws << " (" << draw_pct << "%)\n";
    
    return 0;
}

