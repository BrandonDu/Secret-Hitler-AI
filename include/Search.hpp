#pragma once

#include "Types.hpp"
#include "GameState.hpp"
#include "BeliefState.hpp"
#include "ISMCTS.hpp"
#include "Features.hpp"
#include "BeliefUpdate.hpp"

#include <vector>
#include <iostream>

namespace secret_hitler
{

	void initBeliefs(const GameState &gs, BeliefState &bs, int me);
	GameState determinize(const GameState &gs, const BeliefState &bs, int me, std::mt19937 &rng);

	void selfPlayGen(int round, int games, int myPlayer,
					 std::vector<std::vector<double>> &X_vote, std::vector<int> &Y_vote,
					 std::vector<std::vector<double>> &X_enact, std::vector<int> &Y_enact,
					 std::vector<Role> &recordedRoles, std::mt19937 &rng);

}