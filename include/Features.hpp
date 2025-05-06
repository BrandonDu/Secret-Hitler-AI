#pragma once

#include <vector>
#include "Types.hpp"
#include "GameState.hpp"
#include "BeliefState.hpp"

namespace secret_hitler
{

    std::vector<double> extractVotingFeatures(const GameState &s, const BeliefState &b, int actor);

    std::vector<double> extractEnactFeatures(const GameState &s, int actor);

}
