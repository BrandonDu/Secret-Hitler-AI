#include "Search.hpp"
#include "ProgressBar.hpp"
namespace secret_hitler
{

	void initBeliefs(const GameState &gs, BeliefState &bs, int me)
	{
		auto roles = gs.getRoles();
		for (int j = 0; j < 5; ++j)
		{
			if (j == me)
			{
				bs.pF[j] = (roles[j] == Role::Liberal ? 0.0 : 1.0);
			}
			else if (roles[me] != Role::Liberal)
			{
				bs.pF[j] = (roles[j] != Role::Liberal ? 1.0 : 0.0);
			}
			else
			{
				bs.pF[j] = 0.5;
			}
		}
	}

	GameState determinize(const GameState &rootSt, const BeliefState &B, int myIdx, std::mt19937 &rng)
	{
		GameState st = rootSt;

		std::vector<int> unk;
		for (int i = 0; i < 5; ++i)
			if (i != myIdx)
				unk.push_back(i);
		struct Assign
		{
			int hitler, fascist;
		};
		std::vector<Assign> assigns;
		for (int h = 0; h < (int)unk.size(); ++h)
			for (int f = 0; f < (int)unk.size(); ++f)
				if (f != h)
					assigns.push_back({unk[h], unk[f]});
		std::vector<double> weights(assigns.size());
		double total = 0;
		for (size_t k = 0; k < assigns.size(); ++k)
		{
			double wgt = 1;
			for (int i : unk)
			{
				if (i == assigns[k].hitler || i == assigns[k].fascist)
					wgt *= B.pF[i];
				else
					wgt *= (1 - B.pF[i]);
			}
			weights[k] = wgt;
			total += wgt;
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
					if (!acts.empty() && acts[0].type == ActionType::Vote)
					{
						auto phi = extractFeatures(gs, bs, actor, gs.getRoles()[actor]);
						double p = computeVoteYesProb(phi);
						std::bernoulli_distribution bd(p);
						bool yes = bd(rng);
						for (auto &ac : acts)
							if (ac.voteYes == yes)
							{
								a = ac;
								break;
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
						auto phi = extractFeatures(before, bs, actor, gs.getRoles()[actor]);
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