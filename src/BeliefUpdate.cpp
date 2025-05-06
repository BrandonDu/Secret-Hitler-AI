#include "BeliefUpdate.hpp"
#include "GameState.hpp"
#include "LearnModels.hpp"
#include "Features.hpp"
#include "Types.hpp"

namespace secret_hitler
{

    void updateBelief(BeliefState &B, const GameState &s_before, const Action &a)
    {
        BeliefState Bk;
        Bk.P.fill(0.0);

        for (size_t k = 0; k < B.assignments.size(); ++k)
        {
            GameState sim = s_before;
            auto newRoles = sim.getRoles();
            newRoles.fill(Role::Liberal);

            newRoles[B.assignments[k].hitler] = Role::Hitler;
            newRoles[B.assignments[k].fascist] = Role::Fascist;

            sim.setRoles(newRoles);

            Bk.P[k] = 1.0;
            if (k > 0)
            {
                Bk.P[k - 1] = 0.0;
            }

            double like = 1.0;
            if (a.type == ActionType::Vote)
            {
                auto phi = extractVotingFeatures(sim, Bk, a.actor);

                double pYes = computeVoteYesProb(phi, sim.getRoles()[a.actor]);
                like = a.voteYes ? pYes : (1.0 - pYes);
            }
            else if (a.type == ActionType::Enact)
            {
                auto phi = extractEnactFeatures(s_before, a.actor);
                bool choseF = (s_before.getDrawBuf()[a.index] == Policy::Fascist);
                double pEn = computeEnactFascistProb(phi, sim.getRoles()[a.actor]);
                like = choseF ? pEn : (1.0 - pEn);
            }

            B.P[k] *= like;
        }

        double total = std::accumulate(B.P.begin(), B.P.end(), 0.0);
        if (total > 0.0)
        {
            for (double &pk : B.P)
            {
                pk /= total;
            }
        }
    }
}
