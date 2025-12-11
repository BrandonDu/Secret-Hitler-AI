#include <iostream>
#include <random>
#include <vector>
#include <array>
#include <algorithm>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <string>

#include "GameState.hpp"
#include "ISMCTS_MO.hpp"
#include "BeliefUpdate.hpp"
#include "Search.hpp"
#include "ProgressBar.hpp"
#include "FeatureMode.hpp"
#include "NeuralPolicy.hpp"

using namespace secret_hitler;

struct MatchResult {
    int transformer_wins = 0;
    int manual_wins = 0;
    int draws = 0;
};

MatchResult playHeadToHead(int numGames, int iterations, 
                          FeatureMode transformerMode) {
    MatchResult globalResult;
    std::atomic<int> completed{0};
    std::mutex resultMutex;
    
    constexpr int BAR_WIDTH = 50;
    auto startTime = std::chrono::steady_clock::now();
    
    int numThreads = std::max(1u, std::thread::hardware_concurrency());
    std::vector<std::thread> workers;
    
    // Set feature mode for transformer team
    FeatureMode originalMode = getFeatureMode();
    setFeatureMode(transformerMode);
    
    for (int t = 0; t < numThreads; ++t) {
        workers.emplace_back([&, t, iterations, transformerMode]() {
            std::mt19937 rng(std::random_device{}() + t * 7919);
            MatchResult localResult;
            
            for (int g = t; g < numGames; g += numThreads) {
                GameState gs(rng);
                auto roles = gs.getRoles();
                
                // Split players: 2 use transformer, 2 use manual, 1 random
                std::array<int, 5> seats = {0, 1, 2, 3, 4};
                std::shuffle(seats.begin(), seats.end(), rng);
                
                std::vector<int> transformerSeats = {seats[0], seats[1]};
                std::vector<int> manualSeats = {seats[2], seats[3]};
                int neutralSeat = seats[4];
                
                // Initialize beliefs for all players
                std::array<BeliefState, SH_NUM_PLAYERS> allBeliefs;
                for (int i = 0; i < SH_NUM_PLAYERS; ++i) {
                    initBeliefs(gs, allBeliefs[i], i);
                }
                
                // Create bots
                std::vector<ISMCTS_MO> transformerBots;
                for (int s : transformerSeats) {
                    transformerBots.emplace_back(s);
                }
                
                std::vector<ISMCTS_MO> manualBots;
                for (int s : manualSeats) {
                    manualBots.emplace_back(s);
                }
                
                // Temporarily set mode for transformer team decisions
                FeatureMode savedMode = getFeatureMode();
                
                while (!gs.isTerminal()) {
                    auto legal = gs.getLegalActions();
                    if (legal.empty()) break;
                    
                    int actor = legal.front().actor;
                    Action a;
                    
                    auto it_trans = std::find(transformerSeats.begin(), transformerSeats.end(), actor);
                    auto it_manual = std::find(manualSeats.begin(), manualSeats.end(), actor);
                    
                    if (it_trans != transformerSeats.end()) {
                        // Use transformer mode
                        setFeatureMode(transformerMode);
                        int idx = std::distance(transformerSeats.begin(), it_trans);
                        a = transformerBots[idx].run(gs, allBeliefs, rng, iterations);
                    } else if (it_manual != manualSeats.end()) {
                        // Use manual mode
                        setFeatureMode(FeatureMode::ManualOnly);
                        int idx = std::distance(manualSeats.begin(), it_manual);
                        a = manualBots[idx].run(gs, allBeliefs, rng, iterations);
                    } else {
                        // Neutral player - use random
                        std::uniform_int_distribution<int> dist(0, legal.size() - 1);
                        a = legal[dist(rng)];
                    }
                    
                    // Restore mode
                    setFeatureMode(savedMode);
                    
                    // Update beliefs
                    GameState before = gs;
                    for (int p = 0; p < SH_NUM_PLAYERS; ++p)
                        updateBelief(allBeliefs[p], before, a);
                    
                    gs.apply(a, rng);
                }
                
                int winner = gs.getWinner();
                
                // Count wins for each team
                int transformer_score = 0;
                int manual_score = 0;
                
                for (int s : transformerSeats) {
                    bool isLib = (roles[s] == Role::Liberal);
                    if ((winner > 0 && isLib) || (winner < 0 && !isLib))
                        transformer_score++;
                }
                
                for (int s : manualSeats) {
                    bool isLib = (roles[s] == Role::Liberal);
                    if ((winner > 0 && isLib) || (winner < 0 && !isLib))
                        manual_score++;
                }
                
                if (transformer_score > manual_score)
                    localResult.transformer_wins++;
                else if (manual_score > transformer_score)
                    localResult.manual_wins++;
                else
                    localResult.draws++;
                
                int done = ++completed;
                if (done % 10 == 0 || done == numGames)
                    printProgressBar("Head-to-Head", done, numGames, BAR_WIDTH, startTime);
            }
            
            std::lock_guard<std::mutex> lock(resultMutex);
            globalResult.transformer_wins += localResult.transformer_wins;
            globalResult.manual_wins += localResult.manual_wins;
            globalResult.draws += localResult.draws;
        });
    }
    
    for (auto &th : workers)
        th.join();
    
    // Restore original mode
    setFeatureMode(originalMode);
    
    std::cout << "\n";
    return globalResult;
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <num_games> <iterations> <transformer_mode>\n";
        std::cerr << "  transformer_mode: transformer|both\n";
        return 1;
    }
    
    int numGames = std::stoi(argv[1]);
    int iterations = std::stoi(argv[2]);
    std::string modeStr = argv[3];
    
    FeatureMode transformerMode = FeatureMode::TransformerOnly;
    if (modeStr == "both")
        transformerMode = FeatureMode::Both;
    else if (modeStr != "transformer") {
        std::cerr << "transformer_mode must be 'transformer' or 'both'\n";
        return 1;
    }
    
    constexpr int INPUT_DIM = 32;
    trainingLoggerInit(INPUT_DIM);
    
    if (transformerMode != FeatureMode::ManualOnly) {
        try {
            initTransformerModel("src/sh_transformer.pt", INPUT_DIM);
        } catch (const std::exception &e) {
            std::cerr << "Failed to load transformer: " << e.what() << "\n";
            return 1;
        }
    }
    
    std::cout << "=== ISMCTS_MO Transformer vs Manual (Head-to-Head) ===\n";
    std::cout << "Games: " << numGames << ", Iterations: " << iterations 
              << ", Transformer Mode: " << modeStr << "\n\n";
    
    MatchResult result = playHeadToHead(numGames, iterations, transformerMode);
    
    int total = result.transformer_wins + result.manual_wins + result.draws;
    double transformer_pct = 100.0 * result.transformer_wins / total;
    double manual_pct = 100.0 * result.manual_wins / total;
    double draw_pct = 100.0 * result.draws / total;
    
    std::cout << "\nResults:\n";
    std::cout << "Transformer: " << result.transformer_wins << " wins (" 
              << transformer_pct << "%)\n";
    std::cout << "Manual: " << result.manual_wins << " wins (" 
              << manual_pct << "%)\n";
    std::cout << "Draws: " << result.draws << " (" << draw_pct << "%)\n";
    
    std::cout << "\nTransformer win rate: " << transformer_pct << "%\n";
    
    return 0;
}


