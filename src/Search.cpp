#include "Search.hpp"
#include <numeric>
#include <chrono>
#include <iostream>
#include <atomic>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "ISMCTS_MO.hpp"
#include "Features.hpp"
#include "BeliefUpdate.hpp"
#include "ProgressBar.hpp"
#include "TrainingLogger.hpp"

namespace secret_hitler
{

void initBeliefs(const GameState &gs, BeliefState &bs, int me)
{
    auto roles = gs.getRoles();
    Role myRole = roles[me];
    double totalWeight = 0.0;
    for (int k = 0; k < (int)BeliefState::assignments.size(); ++k)
    {
        const auto &A = BeliefState::assignments[k];
        bool valid = false;
        switch (myRole)
        {
        case Role::Liberal:
            valid = (A.hitler != me && A.fascist != me);
            break;
        case Role::Hitler:
            valid = (A.hitler == me && roles[A.fascist] == Role::Fascist);
            break;
        case Role::Fascist:
            valid = (A.fascist == me && roles[A.hitler] == Role::Hitler);
            break;
        }
        if (valid)
        {
            bs.P[k] = 1.0;
            totalWeight += 1.0;
        }
        else
        {
            bs.P[k] = 0.0;
        }
    }
    if (totalWeight > 0.0)
    {
        for (double &pk : bs.P)
            pk /= totalWeight;
    }
}

GameState determinize(const GameState &rootSt, const BeliefState &B, int observerIdx, std::mt19937 &rng)
{
    GameState st = rootSt;
    auto pH = B.marginalHitler();
    auto pF = B.marginalFascist();

    std::vector<int> unk;
    unk.reserve(SH_NUM_PLAYERS - 1);
    for (int i = 0; i < SH_NUM_PLAYERS; ++i)
        if (i != observerIdx)
            unk.push_back(i);

    std::vector<Assign> assigns;
    assigns.reserve(unk.size() * (unk.size() - 1));
    for (int h = 0; h < (int)unk.size(); ++h)
        for (int f = 0; f < (int)unk.size(); ++f)
            if (f != h)
                assigns.push_back({unk[h], unk[f]});

    std::vector<double> weights(assigns.size());
    double total = 0.0;
    for (size_t k = 0; k < assigns.size(); ++k)
    {
        int h = assigns[k].hitler;
        int f = assigns[k].fascist;
        double w = pH[h] * pF[f];
        for (int i : unk)
            if (i != h && i != f)
                w *= (1.0 - pH[i] - pF[i]);
        weights[k] = w;
        total += w;
    }

    std::uniform_real_distribution<double> dist(0.0, total > 0.0 ? total : 1.0);
    double draw = dist(rng);
    double acc = 0.0;
    size_t chosen = 0;
    for (size_t k = 0; k < weights.size(); ++k)
    {
        acc += weights[k];
        if (draw <= acc)
        {
            chosen = k;
            break;
        }
    }

    auto roles = st.getRoles();
    for (int i : unk)
        roles[i] = Role::Liberal;
    roles[assigns[chosen].hitler] = Role::Hitler;
    roles[assigns[chosen].fascist] = Role::Fascist;
    for (int seat = 0; seat < SH_NUM_PLAYERS; ++seat)
        st.setRole(seat, roles[seat]);

    return st;
}

void selfPlayGen(int games,
                 int myPlayer,
                 std::vector<std::vector<double>> &X_vote,
                 std::vector<int> &Y_vote,
                 std::vector<std::vector<double>> &X_enact,
                 std::vector<int> &Y_enact,
                 std::vector<Role> &recordedRoles,
                 std::mt19937 &rng)
{
    constexpr int BAR_WIDTH = 50;
    auto startTime = std::chrono::steady_clock::now();

    std::vector<uint64_t> seeds(games);
    {
        uint64_t base = std::uniform_int_distribution<uint64_t>()(rng);
        for (int g = 0; g < games; ++g)
        {
            base += 0x9E3779B97F4A7C15ULL;
            uint64_t mix = base ^ (uint64_t)rng();
            mix ^= mix >> 33;
            mix *= 0xff51afd7ed558ccdULL;
            mix ^= mix >> 33;
            mix *= 0xc4ceb9fe1a85ec53ULL;
            mix ^= mix >> 33;
            seeds[g] = mix;
        }
    }

    std::atomic<int> completed{0};

#pragma omp parallel for schedule(dynamic)
    for (int g = 0; g < games; ++g)
    {
        std::mt19937 rng_local(static_cast<uint32_t>(seeds[g]));
        ISMCTS_MO mcts_local(myPlayer);

        std::vector<std::vector<double>> X_vote_local;
        std::vector<int> Y_vote_local;
        std::vector<std::vector<double>> X_enact_local;
        std::vector<int> Y_enact_local;
        std::vector<Role> recordedRoles_local;

        GameState gs(rng_local);
        std::array<BeliefState, SH_NUM_PLAYERS> beliefs;
        for (int i = 0; i < SH_NUM_PLAYERS; ++i)
            initBeliefs(gs, beliefs[i], i);

        while (!gs.isTerminal())
        {
            GameState before = gs;
            auto leg = gs.getLegalActions();
            if (leg.empty())
                break;

            int actor = leg.front().actor;
            Action a;

            if (actor == myPlayer)
            {
                a = mcts_local.run(gs, beliefs, rng_local, 100);
            }
            else
            {
                GameState det = determinize(gs, beliefs[actor], actor, rng_local);
                auto detActs = det.getLegalActions();

                if (!detActs.empty() && detActs.front().type == ActionType::Vote)
                {
                    auto phi = extractVotingFeatures(det, beliefs[actor], actor);
                    double p = computeVoteYesProb(phi, det.getRoles()[actor]);
                    bool yes = std::bernoulli_distribution(p)(rng_local);
                    bool found = false;
                    for (auto &ac : leg)
                    {
                        if (ac.type == ActionType::Vote && ac.voteYes == yes)
                        {
                            a = ac;
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                        a = leg.front();
                }
                else if (!detActs.empty() && detActs.front().type == ActionType::Enact)
                {
                    std::vector<Action> enactActs;
                    enactActs.reserve(leg.size());
                    for (auto &ac : leg)
                        if (ac.type == ActionType::Enact)
                            enactActs.push_back(ac);

                    auto phi = extractEnactFeatures(det);
                    double pF = computeEnactFascistProb(phi, det.getRoles()[actor]);
                    bool chooseF = std::bernoulli_distribution(pF)(rng_local);

                    bool applied = false;
                    for (auto &ac : enactActs)
                    {
                        Policy c = before.getDrawBuf()[ac.index];
                        if ((c == Policy::Fascist) == chooseF)
                        {
                            a = ac;
                            applied = true;
                            break;
                        }
                    }
                    if (!applied)
                    {
                        if (!enactActs.empty())
                            a = enactActs.front();
                        else
                            a = leg.front();
                    }
                }
                else if (!detActs.empty() && detActs.front().type == ActionType::DrawDiscard)
                {
                    bool discarded = false;
                    if (det.getRoles()[actor] == Role::Liberal)
                    {
                        for (auto &ac : leg)
                        {
                            if (before.getDrawBuf()[ac.index] == Policy::Fascist)
                            {
                                a = ac;
                                discarded = true;
                                break;
                            }
                        }
                    }
                    else
                    {
                        for (auto &ac : leg)
                        {
                            if (before.getDrawBuf()[ac.index] == Policy::Liberal)
                            {
                                a = ac;
                                discarded = true;
                                break;
                            }
                        }
                    }
                    if (!discarded)
                        a = leg.front();
                }
                else
                {
                    std::uniform_int_distribution<int> ud(0, (int)leg.size() - 1);
                    a = leg[ud(rng_local)];
                }
            }

            if (actor == myPlayer)
            {
                if (a.type == ActionType::Vote)
                {
                    auto det2 = determinize(before, beliefs[actor], actor, rng_local);
                    auto phi = extractVotingFeatures(det2, beliefs[actor], actor);
                    int y = a.voteYes ? 1 : 0;

                    X_vote_local.push_back(phi);
                    Y_vote_local.push_back(y);

                    int roleInt = static_cast<int>(gs.getRoles()[actor]);
#ifdef _OPENMP
#pragma omp critical
#endif
                    {
                        trainingLoggerRecord(phi, y, roleInt, 1.0);
                    }
                }
                else if (a.type == ActionType::Enact)
                {
                    auto det2 = determinize(before, beliefs[actor], actor, rng_local);
                    auto phi = extractEnactFeatures(det2);
                    Policy chosen = before.getDrawBuf()[a.index];
                    int y = (chosen == Policy::Fascist ? 1 : 0);

                    X_enact_local.push_back(phi);
                    Y_enact_local.push_back(y);
                    recordedRoles_local.push_back(gs.getRoles()[actor]);

                    int roleInt = static_cast<int>(gs.getRoles()[actor]);
#ifdef _OPENMP
#pragma omp critical
#endif
                    {
                        trainingLoggerRecord(phi, y, roleInt, 0.0);
                    }
                }
            }

            gs.apply(a, rng_local);
            for (int p = 0; p < SH_NUM_PLAYERS; ++p)
                updateBelief(beliefs[p], before, a);
        }

#pragma omp critical
        {
            X_vote.insert(X_vote.end(),
                          std::make_move_iterator(X_vote_local.begin()),
                          std::make_move_iterator(X_vote_local.end()));
            Y_vote.insert(Y_vote.end(), Y_vote_local.begin(), Y_vote_local.end());

            X_enact.insert(X_enact.end(),
                           std::make_move_iterator(X_enact_local.begin()),
                           std::make_move_iterator(X_enact_local.end()));
            Y_enact.insert(Y_enact.end(), Y_enact_local.begin(), Y_enact_local.end());

            recordedRoles.insert(recordedRoles.end(),
                                 recordedRoles_local.begin(),
                                 recordedRoles_local.end());
        }

        int done = ++completed;
        if ((done % 5 == 0) || done == games)
        {
            printProgressBar("Self-play", done, games, BAR_WIDTH, startTime);
        }
    }

    std::cout << "\n";
}

}
