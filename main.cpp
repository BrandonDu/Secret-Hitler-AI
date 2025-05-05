#include "GameState.hpp"
#include "ISMCTS.hpp"
#include "Search.hpp"
#include "LearnModels.hpp"
#include "BeliefUpdate.hpp"
#include "Features.hpp"

using namespace secret_hitler;

int main()
{
    std::mt19937 rng(std::random_device{}());
    int voteDim = extractFeatures(GameState(rng), 0, Role::Liberal).size();
    int enactDim = extractEnactFeatures(GameState(rng), 0).size();

    LR_w_yes.assign(voteDim, 0.0);
    LR_b_yes = 0.0;
    loadWeights("weights.txt");

    LR_w_enact_F.assign(enactDim, 0.0);
    LR_b_enact_F = 0.0;
    LR_w_enact_L.assign(enactDim, 0.0);
    LR_b_enact_L = 0.0;
    loadEnactWeights("weights_enact_F.txt", "weights_enact_L.txt");

    std::vector<std::vector<double>> X_vote;
    std::vector<int> Y_vote;
    std::vector<std::vector<double>> X_enact;
    std::vector<int> Y_enact;
    std::vector<Role> recordedRoles;

    int rounds = 5, gamesPerRound = 200, epochs = 5;
    double lr_rate = 0.01;
    int myIndex = 0;

    for (int r = 0; r < rounds; ++r)
    {
        X_vote.clear();
        Y_vote.clear();
        X_enact.clear();
        Y_enact.clear();
        recordedRoles.clear();

        selfPlayGen(gamesPerRound, myIndex, X_vote, Y_vote, X_enact, Y_enact, recordedRoles, rng);

        trainLogRegSGD(X_vote, Y_vote, LR_w_yes, LR_b_yes, lr_rate, epochs);
        saveWeights("weights.txt");

        trainEnactModels(X_enact, Y_enact, recordedRoles, lr_rate, epochs);
        saveEnactWeights(
            "weights_enact_F.txt", LR_b_enact_F, LR_w_enact_F,
            "weights_enact_L.txt", LR_b_enact_L, LR_w_enact_L);
    }

    int liberalWins = 0;
    ISMCTS tester(myIndex);
    const int testGames = 1000;
    for (int g = 0; g < testGames; ++g)
    {
        GameState gs(rng);
        BeliefState bs;
        initBeliefs(gs, bs, myIndex);
        while (!gs.isTerminal())
        {
            auto leg = gs.getLegalActions();
            int actor = leg.front().actor;
            Action a;
            if (actor == myIndex)
            {
                a = tester.run(gs, bs, rng, 500);
            }
            else
            {
                std::vector<Action> acts;
                for (auto &ac : leg)
                    if (ac.actor == actor)
                        acts.push_back(ac);
                if (!acts.empty() && acts[0].type == ActionType::Vote)
                {
                    auto phi = extractFeatures(gs, actor, gs.getRoles()[actor]);
                    std::bernoulli_distribution bd(sigmoid(LR_b_yes + std::inner_product(LR_w_yes.begin(), LR_w_yes.end(), phi.begin(), 0.0)));
                    bool yes = bd(rng);
                    for (auto &ac : acts)
                        if (ac.voteYes == yes)
                        {
                            a = ac;
                            break;
                        }
                }
                else
                {
                    std::uniform_int_distribution<int> ud(0, (int)acts.size() - 1);
                    a = acts.empty() ? Action() : acts[ud(rng)];
                }
            }
            if (a.type == ActionType::Vote)
                updateBelief(bs, gs, a);
            gs.apply(a, rng);
        }
        if (gs.getWinner() > 0)
            ++liberalWins;
    }
    std::cout << "Liberal wins: " << liberalWins << "/" << testGames << std::endl;
    return 0;
}
