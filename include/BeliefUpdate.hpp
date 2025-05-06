#pragma once

#include "BeliefState.hpp"
#include "Types.hpp"
#include "GameState.hpp"
#include "LearnModels.hpp"
#include <vector>

namespace secret_hitler
{

    void updateBelief(BeliefState &B, const GameState &s_before, const Action &a);
    
    inline double sigmoid(double z)
    {
        return 1.0 / (1.0 + std::exp(-z));
    }

    inline double computeVoteYesProb(const std::vector<double> &phi, Role r)
    {
        if (r == Role::Fascist || r == Role::Hitler)
        {
            double z = LR_b_vote_F + std::inner_product(LR_w_vote_F.begin(),
                                                        LR_w_vote_F.end(),
                                                        phi.begin(), 0.0);
            return sigmoid(z);
        }
        else
        {
            double z = LR_b_vote_L + std::inner_product(LR_w_vote_L.begin(),
                                                        LR_w_vote_L.end(),
                                                        phi.begin(), 0.0);
            return sigmoid(z);
        }
    }

    inline double computeEnactFascistProb(const std::vector<double> &phi, Role r)
    {
        if (r == Role::Fascist || r == Role::Hitler)
        {
            double z = LR_b_enact_F + std::inner_product(
                                          LR_w_enact_F.begin(),
                                          LR_w_enact_F.end(),
                                          phi.begin(), 0.0);
            return sigmoid(z);
        }
        else
        {
            return 0.0;
        }
    }

}