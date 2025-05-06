#include "GameState.hpp"
#include "ISMCTS.hpp"
#include "Search.hpp"
#include "LearnModels.hpp"
#include "BeliefUpdate.hpp"
#include "Features.hpp"
#include "ProgressBar.hpp"

using namespace secret_hitler;

int main()
{
    std::mt19937 rng(std::random_device{}());
    int voteDim = extractVotingFeatures(GameState(rng), BeliefState(), 0).size();
    int enactDim = extractEnactFeatures(GameState(rng), 0).size();

    LR_w_vote_F.assign(voteDim, 0.0);
    LR_b_vote_F = 0.0;
    LR_w_vote_L.assign(voteDim, 0.0);
    LR_b_vote_L = 0.0;

    loadVoteWeights("weights_vote_L.txt", "weights_vote_F.txt");

    LR_w_enact_F.assign(enactDim, 0.0);
    LR_b_enact_F = 0.0;

    loadEnactWeights("weights_enact_F.txt", "weights_enact_L.txt");

    std::vector<std::vector<double>> X_vote;
    std::vector<int> Y_vote;
    std::vector<std::vector<double>> X_enact;
    std::vector<int> Y_enact;
    std::vector<Role> recordedRoles;

    int rounds = 30, gamesPerRound = 1000, epochs = 5;
    double lr_rate = 0.02;
    int myIndex = 0;
    constexpr int W = 50;

    for (int r = 0; r < rounds; ++r)
    {
        X_vote.clear();
        Y_vote.clear();
        X_enact.clear();
        Y_enact.clear();
        recordedRoles.clear();
        std::cout << "=== Round " << (r + 1)
                  << ": Self-play (" << gamesPerRound << " games) ===" << std::endl;

        selfPlayGen(r, gamesPerRound, myIndex, X_vote, Y_vote, X_enact, Y_enact, recordedRoles, rng);


        std::vector<std::vector<double>> X_L, X_F;
        std::vector<int>                 Y_L, Y_F;
        X_L.reserve(X_vote.size());
        X_F.reserve(X_vote.size());
        Y_L.reserve(Y_vote.size());
        Y_F.reserve(Y_vote.size());
        for (size_t i = 0; i < X_vote.size(); ++i) {
            if (recordedRoles[i] == Role::Liberal) {
                X_L.push_back(X_vote[i]);
                Y_L.push_back(Y_vote[i]);
            } else {
                X_F.push_back(X_vote[i]);
                Y_F.push_back(Y_vote[i]);
            }
        }

        std::cout << "--- Training vote model (SGD) ---\n";
        trainLogRegSGD(X_L, Y_L, LR_w_vote_L, LR_b_vote_L, lr_rate, epochs);
        trainLogRegSGD(X_F, Y_F, LR_w_vote_F, LR_b_vote_F, lr_rate, epochs);
        saveVoteWeights("weights_vote_L.txt",
                        "weights_vote_F.txt");

        std::cout << "--- Training enact models ---\n";
        trainEnactModels(X_enact, Y_enact, recordedRoles, lr_rate, epochs);
        saveEnactWeights(
            "weights_enact_F.txt",
            "weights_enact_L.txt");
        std::cout << std::endl;
    }

    int liberalWins = 0;
    ISMCTS tester(myIndex);
    constexpr int testGames = 1000;
    constexpr int BAR_WIDTH = 50;

    std::cout << "\n--- Testing trained models ---" << std::endl;
    auto startTime = std::chrono::steady_clock::now();

    printProgressBar("Testing",
                     0,
                     testGames,
                     BAR_WIDTH,
                     startTime);

    for (int g = 0; g < testGames; ++g)
    {
        GameState gs(rng);
        std::array<ISMCTS,5> bots = {
            ISMCTS(0), ISMCTS(1), ISMCTS(2), ISMCTS(3), ISMCTS(4)
        };
        std::array<BeliefState,5> beliefs;
        
        for (int i = 0; i < 5; ++i) {
            initBeliefs(gs, beliefs[i], i);
        }
        
        while (!gs.isTerminal()) {
            auto leg = gs.getLegalActions();
            int actor = leg.front().actor;
            Action a;

            a = bots[actor].run(gs, beliefs[actor], rng, 1000);
        
            for (int i = 0; i < 5; ++i) {
            if (leg.front().type == ActionType::Vote || leg.front().type == ActionType::Enact)
                updateBelief(beliefs[i], gs, a);
            }
            gs.apply(a, rng);
        }
        if (gs.getWinner() > 0)
            ++liberalWins;
        printProgressBar("Testing",
                         g + 1,
                         testGames,
                         BAR_WIDTH,
                         startTime);
    }
    std::cout << "\nLiberal wins: " << liberalWins << "/" << testGames << std::endl;
    return 0;
}
