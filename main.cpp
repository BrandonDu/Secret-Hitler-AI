#include "GameState.hpp"
#include "ISMCTS.hpp"
#include "Search.hpp"
#include "LearnModels.hpp"
#include "BeliefUpdate.hpp"
#include "Features.hpp"
#include "ProgressBar.hpp"
#include "Bots.hpp"
#include "Test.hpp"

using namespace secret_hitler;

int main(int argc, char* argv[]) {
    std::string mode;

    int numRounds=0, numGamesPerRound=0, numEpochs=0, numTestingGames=0;

    int evalGames=0, mctsIters=0;
    std::vector<std::string> opponents;

    for(int i=1;i<argc;++i){
        std::string a=argv[i];
        if(a=="--mode" && i+1<argc)       mode=argv[++i];
        else if(a=="--rounds"&&i+1<argc)  numRounds        =std::stoi(argv[++i]);
        else if(a=="--games-per-round"&&i+1<argc) numGamesPerRound=std::stoi(argv[++i]);
        else if(a=="--epochs"&&i+1<argc)  numEpochs        =std::stoi(argv[++i]);
        else if(a=="--test-games"&&i+1<argc) numTestingGames =std::stoi(argv[++i]);
        else if(a=="--games"&&i+1<argc)   evalGames        =std::stoi(argv[++i]);
        else if(a=="--iters"&&i+1<argc)   mctsIters       =std::stoi(argv[++i]);
        else if(a=="--opponent"&&i+1<argc){
            while(i+1<argc && argv[i+1][0]!='-')
                opponents.emplace_back(argv[++i]);
        }
        else {
            std::cerr<<"Unknown flag "<<a<<"\n";
            return 1;
        }
    }

    if(mode=="train"){
        if(numRounds<=0||numGamesPerRound<=0||numEpochs<=0||numTestingGames<=0){
            std::cerr<<"Usage (train): --mode train "
                     "--rounds N --games-per-round M --epochs E --test-games T\n";
            return 1;
        }
        std::cout<<"[TRAIN] Rnds="<<numRounds
                 <<" G/Rnd="<<numGamesPerRound
                 <<" Epochs="<<numEpochs
                 <<" TestG="<<numTestingGames<<"\n";
        trainTest(numRounds, numGamesPerRound, numEpochs, numTestingGames);
    }
    else if(mode=="evaluate"){
        if(evalGames<=0||mctsIters<=0||opponents.empty()){
            std::cerr<<"Usage (evaluate): --mode evaluate "
                     "--games N --iters X --opponent name1 name2\n";
            return 1;
        }
        for(auto &opp:opponents){
            MatchResult res;
            std::string label="MCTS vs "+opp;
            if(opp=="random")  res=play<RandomBot>(evalGames,mctsIters,label.c_str());
            else if(opp=="greedy") res=play<GreedyBot>(evalGames,mctsIters,label.c_str());
            else {
                std::cerr<<"Unknown opponent "<<opp<<"\n";
                continue;
            }
            int total=res.mctsWins+res.oppWins;
            double pct=100.0*res.mctsWins/total;
            std::cout<<label<<" ("<<mctsIters<<" iters): "<<pct<<"%\n";
        }
    }
    else {
        std::cerr<<"Please specify --mode train|evaluate\n";
        return 1;
    }
    return 0;
}