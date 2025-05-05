#pragma once

#include <vector>
#include "Types.hpp"
#include "GameState.hpp"
#include "BeliefState.hpp"

namespace secret_hitler
{

    inline double sigmoid(double z)
    {
        return 1.0 / (1.0 + std::exp(-z));
    }

    std::vector<double> extractFeatures(const GameState &s, const BeliefState &b, int actor, Role r);

    std::vector<double> extractEnactFeatures(const GameState &s, int actor);

}
