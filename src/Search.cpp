#include "Search.hpp"
#include "ProgressBar.hpp"

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

		if (totalWeight > 0)
		{
			for (double &pk : bs.P)
			{
				pk /= totalWeight;
			}
		}
	}

	GameState determinize(const GameState &rootSt, const BeliefState &B, int myIdx, std::mt19937 &rng)
	{
		GameState st = rootSt;
		auto pH = B.marginalHitler();
		auto pF = B.marginalFascist();

		std::vector<int> unk;
		for (int i = 0; i < 5; ++i)
			if (i != myIdx)
				unk.push_back(i);
		std::vector<Assign> assigns;
		for (int h = 0; h < (int)unk.size(); ++h)
			for (int f = 0; f < (int)unk.size(); ++f)
				if (f != h)
					assigns.push_back({unk[h], unk[f]});
		std::vector<double> weights(assigns.size());

		double total = 0;
		for (size_t k = 0; k < assigns.size(); ++k)
		{
			int h = assigns[k].hitler, f = assigns[k].fascist;
			double w = pH[h] * pF[f];
			for (int i : unk)
				if (i != h && i != f)
					w *= (1.0 - pH[i] - pF[i]);
			weights[k] = w;
			total += w;
		}
		std::uniform_real_distribution<double> dist(0.0, total);
		double draw = dist(rng), acc = 0;
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
		return st;
	}

	void selfPlayGen(int round,
					 int games,
					 int myPlayer,
					 std::vector<std::vector<double>> &X_vote,
					 std::vector<int> &Y_vote,
					 std::vector<std::vector<double>> &X_enact,
					 std::vector<int> &Y_enact,
					 std::vector<Role> &recordedRoles,
					 std::mt19937 &rng)
	{
		ISMCTS mcts(myPlayer);
		constexpr int BAR_WIDTH = 50;
		auto startTime = std::chrono::steady_clock::now();

		for (int g = 0; g < games; ++g)
		{
			GameState gs(rng);
			BeliefState bs;
			initBeliefs(gs, bs, myPlayer);
			auto roles = gs.getRoles();

			while (!gs.isTerminal())
			{
				GameState before = gs;
				auto leg = gs.getLegalActions();
				int actor = leg.front().actor;
				Action a;
				if (actor == myPlayer)
				{
					a = mcts.run(gs, bs, rng, 100);
				}
				else
				{
					std::vector<Action> acts;
					for (auto &ac : leg)
						if (ac.actor == actor)
							acts.push_back(ac);
					if (acts[0].type == ActionType::Vote)
					{
						auto phi = extractVotingFeatures(gs, bs, actor);
						double p = computeVoteYesProb(phi, roles[actor]);
						std::bernoulli_distribution bd(p);
						bool yes = bd(rng);
						for (auto &ac : acts)
							if (ac.voteYes == yes)
							{
								a = ac;
								break;
							}
					}
					else if (acts[0].type == ActionType::Enact)
					{
						std::vector<Action> enactActs;
						for (auto &ac : acts)
							if (ac.type == ActionType::Enact)
								enactActs.push_back(ac);

						if (roles[actor] == Role::Liberal)
						{
							bool applied = false;
							auto drawBuf = before.getDrawBuf();
							for (auto &ac : enactActs)
							{
								if (drawBuf[ac.index] == Policy::Liberal)
								{
									a = ac;
									applied = true;
									break;
								}
							}
							if (!applied)
							{
								for (auto &ac : enactActs)
								{
									if (drawBuf[ac.index] == Policy::Fascist)
									{
										a = ac;
										break;
									}
								}
							}
						}
						else
						{
							auto phi = extractEnactFeatures(before, actor);
							double pF = computeEnactFascistProb(phi, gs.getRoles()[actor]);

							bool chooseF = std::bernoulli_distribution(pF)(rng);

							for (auto &ac : enactActs)
							{
								Policy c = before.getDrawBuf()[ac.index];
								if ((c == Policy::Fascist) == chooseF)
								{
									a = ac;
									break;
								}
							}
						}
					}
					else
					{
						std::uniform_int_distribution<int> ud(0, (int)leg.size() - 1);
						a = leg[ud(rng)];
					}
				}
				if (actor == myPlayer)
				{
					if (a.type == ActionType::Vote)
					{
						auto phi = extractVotingFeatures(before, bs, actor);
						X_vote.push_back(phi);
						Y_vote.push_back(a.voteYes ? 1 : 0);
					}
					else if (a.type == ActionType::Enact)
					{
						auto phi = extractEnactFeatures(before, actor);
						X_enact.push_back(phi);
						Policy chosen = before.getDrawBuf()[a.index];
						Y_enact.push_back(chosen == Policy::Fascist ? 1 : 0);
						recordedRoles.push_back(gs.getRoles()[actor]);
					}
				}
				if (a.type == ActionType::Vote)
					updateBelief(bs, before, a);
				gs.apply(a, rng);
			}
			printProgressBar("Self-play",
							 g + 1,
							 games,
							 BAR_WIDTH,
							 startTime);
		}
		std::cout << "\n";
	}

}