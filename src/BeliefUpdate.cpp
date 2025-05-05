#include "BeliefUpdate.hpp"
#include "GameState.hpp"
#include "LearnModels.hpp"
#include "Features.hpp"

namespace secret_hitler
{

    void updateBelief(BeliefState &B, const GameState &s_before, const Action &a)
    {
        int j = a.actor;
        if (B.pF[j] <= 0.0 || B.pF[j] >= 1.0)
            return;

        if (a.type == ActionType::Vote)
        {
            auto phiF = extractFeatures(s_before, j, Role::Fascist);
            auto phiL = extractFeatures(s_before, j, Role::Liberal);
            double pF = computeVoteYesProb(phiF);
            double pL = computeVoteYesProb(phiL);
            double likeF = a.voteYes ? pF : (1 - pF);
            double likeL = a.voteYes ? pL : (1 - pL);
            double postF = likeF * B.pF[j];
            double postL = likeL * (1 - B.pF[j]);
            B.pF[j] = postF / (postF + postL);
        }
        else if (a.type == ActionType::Enact)
        {
            auto phi = extractEnactFeatures(s_before, j);

            double zF = LR_b_enact_F;
            double zL = LR_b_enact_L;
            for (size_t k = 0; k < phi.size(); ++k)
            {
                zF += LR_w_enact_F[k] * phi[k];
                zL += LR_w_enact_L[k] * phi[k];
            }
            double pEnact_ifF = sigmoid(zF);
            double pEnact_ifL = sigmoid(zL);

            Policy chosen = s_before.getDrawBuf()[a.index];
            double likeF = (chosen == Policy::Fascist ? pEnact_ifF : (1 - pEnact_ifF));
            double likeL = (chosen == Policy::Fascist ? pEnact_ifL : (1 - pEnact_ifL));

            double postF = likeF * B.pF[j];
            double postL = likeL * (1 - B.pF[j]);
            B.pF[j] = postF / (postF + postL);
        }
    }

    double computeVoteYesProb(const std::vector<double> &phi)
    {
        double z = LR_b_yes;
        for (size_t i = 0; i < phi.size(); ++i)
            z += LR_w_yes[i] * phi[i];
        return sigmoid(z);
    }

}
