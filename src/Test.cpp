
#include "Test.hpp"

namespace secret_hitler
{
    void traceOneGame(int mctsIters)
    {
        std::mt19937 rng(std::random_device{}());
        GameState gs(rng);
        auto roles = gs.getRoles();

        std::cout << "=== New Game Trace ===\nSeat roles: ";
        for (int i = 0; i < 5; ++i)
            std::cout << i << "=" << to_string(roles[i]) << "  ";
        std::cout << "\n\n";

        using DecideFn = std::function<Action(GameState &, std::mt19937 &)>;
        std::array<DecideFn, 5> decide;
        std::vector<ISMCTS> mctsBots;
        std::vector<BeliefState> beliefs;

        for (int seat = 0; seat < 5; ++seat)
        {
            if (roles[seat] == Role::Liberal)
            {
                int idx = (int)mctsBots.size();
                mctsBots.emplace_back(seat);
                beliefs.emplace_back();
                initBeliefs(gs, beliefs[idx], seat);
                decide[seat] = [idx, &mctsBots, &beliefs, mctsIters](GameState &st, std::mt19937 &rg)
                {
                    return mctsBots[idx].run(st, beliefs[idx], rg, mctsIters);
                };
            }
            else
            {
                GreedyBot gb(seat);
                decide[seat] = [gb](GameState &st, std::mt19937 &) mutable
                {
                    return gb.act(st);
                };
            }
        }

        int step = 0;
        while (!gs.isTerminal())
        {
            auto leg = gs.getLegalActions();
            auto type = leg.front().type;

            switch (type)
            {

            case ActionType::Nominate:
            {
                int pres = leg.front().actor;
                Action a = decide[pres](gs, rng);

                std::cout << "----------------------------------------\n";
                std::cout << "[Step " << step << "] PRESIDENT (seat "
                          << pres << " / " << to_string(roles[pres]) << ")\n";
                std::cout << "-> Nominate Chancellor: seat "
                          << a.target << "\n";

                gs.apply(a, rng);
                break;
            }

            case ActionType::Vote:
            {
                std::cout << "[Step " << step << "] VOTING\n";
                auto alive = gs.isAlive();

                for (int voter = 0; voter < 5; ++voter)
                {
                    if (!alive[voter])
                        continue;

                    auto leg2 = gs.getLegalActions();
                    std::vector<Action> votes;
                    for (auto &ac : leg2)
                        if (ac.type == ActionType::Vote && ac.actor == voter)
                            votes.push_back(ac);

                    Action a = decide[voter](gs, rng);
                    std::cout << "  [Vote] seat " << voter << " ("
                              << to_string(roles[voter]) << ") -> "
                              << (a.voteYes ? "Y" : "N") << "\n";

                    gs.apply(a, rng);
                    for (auto &b : beliefs)
                        updateBelief(b, gs, a);
                }
                break;
            }

            case ActionType::DrawDiscard:
            {
                int pres = leg.front().actor;
                auto buf = gs.getDrawBuf();

                std::cout << "[Step " << step << "] PRESIDENT DRAW (seat "
                          << pres << " / " << to_string(roles[pres]) << ")\n";
                std::cout << "   Cards: [0]=" << to_string(buf[0])
                          << "  [1]=" << to_string(buf[1])
                          << "  [2]=" << to_string(buf[2]) << "\n";

                Action a = decide[pres](gs, rng);
                std::cout << "   Discard idx " << a.index
                          << " (" << to_string(buf[a.index]) << ")\n";
                std::cout << "----------------------------------------\n";

                gs.apply(a, rng);
                break;
            }

            case ActionType::Enact:
            {
                int chanc = leg.front().actor;
                auto buf = gs.getDrawBuf();
                Action a = decide[chanc](gs, rng);

                std::cout << "[Step " << step << "] CHANCELLOR (seat "
                          << chanc << " / " << to_string(roles[chanc]) << ")\n";
                std::cout << "   Enact idx " << a.index
                          << " (" << to_string(buf[a.index]) << ")\n";

                gs.apply(a, rng);
                for (auto &b : beliefs)
                    updateBelief(b, gs, a);
                break;
            }

            case ActionType::Veto:
            {
                int chanc = leg.front().actor;
                Action a = decide[chanc](gs, rng);
                std::cout << "[Step " << step << "] CHANCELLOR VETO (seat "
                          << chanc << ") -> " << (a.voteYes ? "Accept" : "Reject")
                          << "\n";
                gs.apply(a, rng);
                break;
            }

            case ActionType::Execute:
            {
                int pres = leg.front().actor;
                Action a = decide[pres](gs, rng);
                std::cout << "[Step " << step << "] EXECUTION (seat "
                          << pres << ") kills seat " << a.target << "\n";
                gs.apply(a, rng);
                break;
            }

            default:
            {
                Action a = decide[leg.front().actor](gs, rng);
                gs.apply(a, rng);
                break;
            }
            }

            std::cout << "    [L=" << gs.getEnactedLiberal()
                      << "  F=" << gs.getEnactedFascist()
                      << "  chaos=" << gs.getChaosTrack() << "]\n\n";
            ++step;
        }

        std::cout << ">>> WINNER: "
                  << (gs.getWinner() > 0 ? "Liberal" : "Fascist")
                  << " <<<\n\n";
    }

    void trainTest(const int numRounds, const int numGamesPerRound, const int numEpochs, const int numTestingGames)
    {
        std::mt19937 rng(std::random_device{}());
        int voteDim = extractVotingFeatures(GameState(rng), BeliefState(), 0).size();
        int enactDim = extractEnactFeatures(GameState(rng)).size();

        LR_w_vote_F.assign(voteDim, 0.0);
        LR_b_vote_F = 0.0;
        LR_w_vote_L.assign(voteDim, 0.0);
        LR_b_vote_L = 0.0;

        loadVoteWeights("weights_vote_L.txt", "weights_vote_F.txt");

        LR_w_enact_F.assign(enactDim, 0.0);
        LR_b_enact_F = 0.0;

        loadEnactWeights("weights_enact_F.txt");

        std::vector<std::vector<double>> X_vote;
        std::vector<int> Y_vote;
        std::vector<std::vector<double>> X_enact;
        std::vector<int> Y_enact;
        std::vector<Role> recordedRoles;

        int rounds = numRounds, gamesPerRound = numGamesPerRound, epochs = numEpochs;
        double lr_rate = 0.02;
        int myIndex = 0;

        for (int r = 0; r < rounds; ++r)
        {
            X_vote.clear();
            Y_vote.clear();
            X_enact.clear();
            Y_enact.clear();
            recordedRoles.clear();
            std::cout << "=== Round " << (r + 1)
                      << ": Self-play (" << gamesPerRound << " games) ===" << std::endl;

            selfPlayGen(gamesPerRound, myIndex, X_vote, Y_vote, X_enact, Y_enact, recordedRoles, rng);

            std::vector<std::vector<double>> X_L, X_F;
            std::vector<int> Y_L, Y_F;
            X_L.reserve(X_vote.size());
            X_F.reserve(X_vote.size());
            Y_L.reserve(Y_vote.size());
            Y_F.reserve(Y_vote.size());
            for (size_t i = 0; i < X_vote.size(); ++i)
            {
                if (recordedRoles[i] == Role::Liberal)
                {
                    X_L.push_back(X_vote[i]);
                    Y_L.push_back(Y_vote[i]);
                }
                else
                {
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
        const int testGames = numTestingGames;
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
            std::array<ISMCTS, 5> bots = {
                ISMCTS(0), ISMCTS(1), ISMCTS(2), ISMCTS(3), ISMCTS(4)};
            std::array<BeliefState, 5> beliefs;

            for (int i = 0; i < 5; ++i)
            {
                initBeliefs(gs, beliefs[i], i);
            }

            while (!gs.isTerminal())
            {
                auto leg = gs.getLegalActions();
                int actor = leg.front().actor;
                Action a;

                a = bots[actor].run(gs, beliefs[actor], rng, 1000);

                for (int i = 0; i < 5; ++i)
                {
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
    }
}