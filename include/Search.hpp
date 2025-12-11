#pragma once
#include <array>
#include <random>
#include <vector>
#include "GameState.hpp"
#include "BeliefState.hpp"
#include "Types.hpp"

namespace secret_hitler
{

	void initBeliefs(const GameState &gs, BeliefState &bs, int me);
	GameState determinize(const GameState &rootSt, const BeliefState &B, int observerIdx, std::mt19937 &rng);

	void selfPlayGen(int games,
					 int myPlayer,
					 std::vector<std::vector<double>> &X_vote,
					 std::vector<int> &Y_vote,
					 std::vector<std::vector<double>> &X_enact,
					 std::vector<int> &Y_enact,
					 std::vector<Role> &recordedRoles,
					 std::mt19937 &rng);

}