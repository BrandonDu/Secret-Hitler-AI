#include "Features.hpp"
#include "BeliefState.hpp"
namespace secret_hitler
{

	std::vector<double> extractVotingFeatures(
		const GameState &s,
		const BeliefState &B,
		int voter)
	{
		std::vector<double> phi;

		phi.push_back(s.getEnactedLiberal());
		phi.push_back(s.getEnactedFascist());
		phi.push_back(s.getChaosTrack());
		
		bool lateGame = (s.getEnactedFascist() >= 3);
		phi.push_back( lateGame ? 1.0 : 0.0 ); 

		int president = s.getNextPresident();
		int nominee = s.getNominee();
		
		auto pH = B.marginalHitler();
		auto pF = B.marginalFascist();

		phi.push_back( pH[nominee] );  
		phi.push_back ( pF[nominee] );

		phi.push_back( pH[president]);
		phi.push_back ( pF[president] );

		phi.push_back( pH[voter] + pF[voter] );

		return phi;
	}

	std::vector<double> extractEnactFeatures(const GameState &s, int actor)
	{
		std::vector<double> phi;

		phi.push_back(s.getEnactedLiberal());
		phi.push_back(s.getEnactedFascist());
		phi.push_back(s.getChaosTrack());
		
		return phi;
	}

}