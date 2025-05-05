#include "Features.hpp"

namespace secret_hitler
{

	std::vector<double> extractFeatures(const GameState &s, int actor, Role r)
	{
		std::vector<double> phi;
		phi.push_back((r == Role::Fascist || r == Role::Hitler) ? 1.0 : 0.0);
		phi.push_back(s.getEnactedLiberal());
		phi.push_back(s.getEnactedFascist());
		phi.push_back(s.getChaosTrack());
		for (int i = 0; i < 5; ++i)
			phi.push_back(i == actor ? 1.0 : 0.0);
		return phi;
	}

	std::vector<double> extractEnactFeatures(const GameState &s, int actor)
	{
		std::vector<double> phi;
		auto drawBuf = s.getDrawBuf();
		for (int i = 0; i < 2; ++i)
			phi.push_back((drawBuf[i] == Policy::Fascist) ? 1.0 : 0.0);

		phi.push_back(s.getEnactedLiberal());
		phi.push_back(s.getEnactedFascist());
		phi.push_back(s.getChaosTrack());

		auto roles = s.getRoles();
		phi.push_back((roles[actor] == Role::Fascist || roles[actor] == Role::Hitler) ? 1.0 : 0.0);

		return phi;
	}

}