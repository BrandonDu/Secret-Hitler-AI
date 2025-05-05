#pragma once

#include "BeliefState.hpp"
#include "Types.hpp"
#include "GameState.hpp"

#include <vector>

namespace secret_hitler
{

    void updateBelief(BeliefState &B, const GameState &s_before, const Action &a);
    double computeVoteYesProb(const std::vector<double> &phi);

}