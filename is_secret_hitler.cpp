#include <algorithm>
#include <array>
#include <cmath>
#include <deque>
#include <fstream>
#include <iostream>
#include <random>
#include <utility>
#include <vector>
#include <memory>
#include <cassert>
#include <limits>
#include <numeric>

enum class Policy { Liberal, Fascist };
enum class Role { Liberal, Fascist, Hitler };
enum class ActionType { Nominate, Vote, DrawDiscard, Enact, Veto, Execute };
enum class SpecialWin { None, Liberal, Fascist };

struct Action {
	ActionType type;
	int actor;
	int target;      
	bool voteYes;   
	int index;      
};

struct GameState {
	int enactedLiberal = 0;
	int enactedFascist = 0;
	int chaosTrack = 0;
	int nextPresident = 0;
	int lastChancellor = -1;
	int lastPresident = -1;
	SpecialWin specialWin = SpecialWin::None;
	bool offeredVeto = false;
	enum Phase { Nomination, Voting, Drawing, Enacting, VetoVoting, Execution } phase = Nomination;
	int nominee = -1;

	std::array<Policy,3> drawBuf;
	std::vector<std::pair<int,bool>> votes;
	std::array<Role,5> roles;
	std::deque<Policy> deck;
	std::array<bool,5> alive;

	std::mt19937 rng{std::random_device{}()};

	GameState() {
		std::array<Role,5> r = { Role::Hitler, Role::Fascist, Role::Liberal, Role::Liberal, Role::Liberal };
		std::shuffle(r.begin(), r.end(), rng);
		roles = r;

		std::vector<Policy> tmp;
		tmp.insert(tmp.end(), 6, Policy::Liberal);
		tmp.insert(tmp.end(), 11, Policy::Fascist);
		std::shuffle(tmp.begin(), tmp.end(), rng);
		deck = std::deque<Policy>(tmp.begin(), tmp.end());

		alive.fill(true);
	}

	bool isTerminal() const {
		if (specialWin != SpecialWin::None) return true;
		if (enactedLiberal >= 5) return true;
		if (enactedFascist >= 6) return true;
		return false;
	}

	int getWinner() const {
		if (specialWin == SpecialWin::Liberal) return +1;
		if (specialWin == SpecialWin::Fascist) return -1;
		if (enactedFascist >= 6) return -1;
		if (enactedLiberal >= 5) return +1;
		return 0;
	}

	void replenishDeck() {
		int libs = 6 - enactedLiberal;
		int facs = 11 - enactedFascist;
		std::vector<Policy> pool;
		pool.insert(pool.end(), libs, Policy::Liberal);
		pool.insert(pool.end(), facs, Policy::Fascist);
		std::shuffle(pool.begin(), pool.end(), rng);
		deck = std::deque<Policy>(pool.begin(), pool.end());
	}

	void startDrawing() {
		if (deck.size() < 3)  replenishDeck();
		for (int i = 0; i < 3; ++i) {
			drawBuf[i] = deck.front();
			deck.pop_front();
		}
	}

	std::vector<Action> getLegalActions() const {
		std::vector<Action> out;
		int actor;
		switch (phase) {
		case Nomination: {
			actor = nextPresident;
            std::vector<int> allCand;
            for (int i = 0; i < 5; ++i) {
                if (alive[i] && i != actor)
                    allCand.push_back(i);
            }
            std::vector<int> filtered = allCand;
            if (lastChancellor >= 0) {
                filtered.erase(
                    std::remove(filtered.begin(), filtered.end(), lastChancellor),
                    filtered.end()
                );
            }
            if (lastPresident >= 0) {
                filtered.erase(
                    std::remove(filtered.begin(), filtered.end(), lastPresident),
                    filtered.end()
                );
            }
            const auto& cand = filtered.empty() ? allCand : filtered;
            for (int tgt : cand) {
                out.push_back({ ActionType::Nominate, actor, tgt, false, -1 });
            }
            break;
		}
		case Voting:
			for (int i = 0; i < 5; ++i) {
				if (!alive[i]) continue;
				out.push_back({ActionType::Vote, i, -1, true, -1});
				out.push_back({ActionType::Vote, i, -1, false, -1});
			}
			break;
		case Drawing: {
			actor = nextPresident;
			for (int idx = 0; idx < 3; ++idx)
				out.push_back({ActionType::DrawDiscard, actor, -1, false, idx});
			break;
		}
		case Enacting: {
			actor = nominee;
			for (int idx = 0; idx < 2; ++idx)
				out.push_back({ActionType::Enact, actor, -1, false, idx});
			if (enactedFascist >= 5 && !offeredVeto)
				out.push_back({ActionType::Veto, actor, -1, false, -1});
			break;
		}
		case VetoVoting:
			actor = nextPresident;
			out.push_back({ActionType::Vote, actor, -1, true, -1});
			out.push_back({ActionType::Vote, actor, -1, false, -1});
			break;
		case Execution:
			actor = lastPresident;
			for (int i = 0; i < 5; ++i)
				if (alive[i] && i != actor)
					out.push_back({ActionType::Execute, actor, i, false, -1});
			break;
		}
		return out;
	}

	void apply(const Action &a) {
		auto findNextAlive = [&](int cur) {
			for (int d = 1; d <= 5; ++d) {
				int cand = (cur + d) % 5;
				if (alive[cand]) return cand;
			}
			return cur;
		};

		switch (phase) {
		case Nomination:
			if (a.type == ActionType::Nominate) {
				lastPresident = nextPresident;
				nominee = a.target;
				phase = Voting;
				votes.clear();
			}
			break;

		case Voting:
			if (a.type == ActionType::Vote) {
				votes.emplace_back(a.actor, a.voteYes);
				int aliveCount = std::count(alive.begin(), alive.end(), true);
				if ((int)votes.size() == aliveCount) {
					int yes = 0;
					for (auto &v: votes) if (v.second) ++yes;
					if (yes >= 3) {
						lastChancellor = nominee;
						if (enactedFascist >= 3 && roles[nominee] == Role::Hitler) {
							specialWin = SpecialWin::Fascist;
							phase = Nomination;
						} else {
							phase = Drawing;
							startDrawing();
							chaosTrack = 0;
						}
					} else {
						lastPresident = nextPresident;
						++chaosTrack;
						nextPresident = findNextAlive(nextPresident);
						phase = Nomination;
						if (chaosTrack >= 5) {
							if (deck.empty()) replenishDeck();
							Policy p = deck.front();
							deck.pop_front();
							if (p == Policy::Liberal) ++enactedLiberal;
							else ++enactedFascist;
							chaosTrack = 0;
						}
					}
					votes.clear();
				}
			}
			break;

		case Drawing:
			if (a.type == ActionType::DrawDiscard) {
				std::vector<Policy> remain;
				for (int i = 0; i < 3; ++i)
					if (i != a.index) remain.push_back(drawBuf[i]);
				drawBuf[0] = remain[0];
				drawBuf[1] = remain[1];
				phase = Enacting;
			}
			break;

		case Enacting:
			if (a.type == ActionType::Enact) {
				Policy e = drawBuf[a.index];
				if (e == Policy::Liberal) ++enactedLiberal;
				else ++enactedFascist;
				lastPresident = nextPresident;
				if (e == Policy::Fascist && (enactedFascist == 4 || enactedFascist == 5)) {
					phase = Execution;
				} else {
					nextPresident = findNextAlive(nextPresident);
					phase = Nomination;
				}
			} else if (a.type == ActionType::Veto) {
				phase = VetoVoting;
				votes.clear();
			}
			offeredVeto = false;
			break;

		case VetoVoting:
			if (a.type == ActionType::Vote && a.actor == nextPresident) {
				if (a.voteYes) {
					++chaosTrack;
					if (chaosTrack >= 5) {
						if (deck.empty()) replenishDeck();
						Policy p = deck.front();
						deck.pop_front();
						if (p == Policy::Liberal) ++enactedLiberal;
						else ++enactedFascist;
						chaosTrack = 0;
					}
					lastPresident = nextPresident;
					nextPresident = findNextAlive(nextPresident);
					phase = Nomination;
				} else {
					offeredVeto = true;
					phase = Enacting;
				}
			}
			break;

		case Execution:
			if (a.type == ActionType::Execute) {
				alive[a.target] = false;
				if (roles[a.target] == Role::Hitler)
					specialWin = SpecialWin::Liberal;
				nextPresident = findNextAlive(lastPresident);
				phase = Nomination;
			}
			break;
		}
	}
};

struct BeliefState {
	std::array<double,5> pF;
};
static std::vector<double> LR_w_yes;
static double LR_b_yes;

static std::vector<double> LR_w_enact_F, LR_w_enact_L;
static double LR_b_enact_F, LR_b_enact_L;

inline double sigmoid(double z) {
	return 1.0/(1.0+std::exp(-z));
}

double computeVoteYesProb(const std::vector<double>& phi) {
	double z = LR_b_yes;
	for (size_t i=0; i<phi.size(); ++i) z += LR_w_yes[i]*phi[i];
	return sigmoid(z);
}

std::vector<double> extractFeatures(const GameState &s, int actor, Role r) {
	std::vector<double> phi;
	phi.push_back((r==Role::Fascist||r==Role::Hitler)?1.0:0.0);
	phi.push_back(s.enactedLiberal);
	phi.push_back(s.enactedFascist);
	phi.push_back(s.chaosTrack);
	for (int i=0; i<5; ++i) phi.push_back(i==actor?1.0:0.0);
	return phi;
}

std::vector<double> extractEnactFeatures(const GameState &s, int actor) {
	std::vector<double> phi;
	for (int i=0; i<2; ++i) phi.push_back((s.drawBuf[i]==Policy::Fascist)?1.0:0.0);
	phi.push_back(s.enactedLiberal);
	phi.push_back(s.enactedFascist);
	phi.push_back(s.chaosTrack);
	phi.push_back((s.roles[actor]==Role::Fascist||s.roles[actor]==Role::Hitler)?1.0:0.0);
	return phi;
}


void loadWeights(const std::string &f) {
	std::ifstream in(f);
	if (!in) return;
	in>>LR_b_yes;
	for(auto &w: LR_w_yes) in>>w;
}
void saveWeights(const std::string &f) {
	std::ofstream out(f);
	if(!out) return;
	out<<LR_b_yes;
	for(auto &w:LR_w_yes) out<<' '<<w;
	out<<"\n";
}
void loadEnactWeights(const std::string &fF, const std::string &fL) {
    std::ifstream inF(fF), inL(fL);
    if (!inF || !inL) {
        std::cerr << "Warning: enact weight files not found, starting from zeros\n";
        return;
    }
    inF >> LR_b_enact_F;
    for (auto &w: LR_w_enact_F) inF >> w;
    inL >> LR_b_enact_L;
    for (auto &w: LR_w_enact_L) inL >> w;
}

void saveEnactWeights(
    const std::string &fF, double bF, const std::vector<double> &wF,
    const std::string &fL, double bL, const std::vector<double> &wL
) {
    std::ofstream outF(fF), outL(fL);
    if (outF) {
        outF << bF;
        for (auto &w: wF) outF << ' ' << w;
        outF << '\n';
    }
    if (outL) {
        outL << bL;
        for (auto &w: wL) outL << ' ' << w;
        outL << '\n';
    }
}

void updateBelief(BeliefState &B, const GameState &s_before, const Action &a) {
    int j = a.actor;
    if (B.pF[j] <= 0.0 || B.pF[j] >= 1.0) return;

    if (a.type == ActionType::Vote) {
        auto phiF = extractFeatures(s_before, j, Role::Fascist);
        auto phiL = extractFeatures(s_before, j, Role::Liberal);
        double pF = computeVoteYesProb(phiF);
        double pL = computeVoteYesProb(phiL);
        double likeF = a.voteYes ? pF : (1 - pF);
        double likeL = a.voteYes ? pL : (1 - pL);
        double postF = likeF * B.pF[j];
        double postL = likeL * (1 - B.pF[j]);
        B.pF[j] = postF / (postF + postL);
    }
    else if (a.type == ActionType::Enact) {
        auto phi = extractEnactFeatures(s_before, j);

        double zF = LR_b_enact_F;
        double zL = LR_b_enact_L;
        for (size_t k = 0; k < phi.size(); ++k) {
            zF += LR_w_enact_F[k] * phi[k];   
            zL += LR_w_enact_L[k] * phi[k];    
        }
        double pEnact_ifF = sigmoid(zF);
        double pEnact_ifL = sigmoid(zL);

        Policy chosen = s_before.drawBuf[a.index];
        double likeF = (chosen == Policy::Fascist ? pEnact_ifF : (1 - pEnact_ifF));
        double likeL = (chosen == Policy::Fascist ? pEnact_ifL : (1 - pEnact_ifL));

        double postF = likeF * B.pF[j];
        double postL = likeL * (1 - B.pF[j]);
        B.pF[j] = postF / (postF + postL);
    }
}

void trainLogRegSGD(const std::vector<std::vector<double>>& X,
                    const std::vector<int>& Y,
                    std::vector<double>& w,
                    double& b,
                    double lr,
                    int epochs) {
	int N=(int)X.size(), d=(int)w.size();
	std::mt19937 rng(std::random_device{}());
	std::uniform_int_distribution<int> dist(0,N-1);
	int total=epochs*N, width=50;
	for(int ep=0; ep<epochs; ++ep) for(int it=0; it<N; ++it) {
			int idx=dist(rng);
			double z=b;
			for(int j=0; j<d; ++j) z+=w[j]*X[idx][j];
			double p=1.0/(1.0+std::exp(-z)), err=p-Y[idx];
			for(int j=0; j<d; ++j) w[j]-=lr*err*X[idx][j];
			b-=lr*err;
			int step=ep*N+it+1, pos=int(width*(double(step)/total));
			std::cout<<"\r[Training] [";
			for(int k=0; k<width; ++k) std::cout<<(k<pos?'=':' ');
			std::cout<<"] "<<int(double(step)/total*100)<<"%"<<std::flush;
		}
	std::cout<<"\n";
}

void trainEnactModels(
    const std::vector<std::vector<double>>& X_enact,
    const std::vector<int>& Y_enact,
    const std::vector<Role>& roles,
    double lr,
    int epochs)
{
    assert(X_enact.size()==Y_enact.size());
    std::vector<std::vector<double>> X_F, X_L;
    std::vector<int> Y_F, Y_L;
    X_F.reserve(X_enact.size()); Y_F.reserve(Y_enact.size());
    X_L.reserve(X_enact.size()); Y_L.reserve(Y_enact.size());
    for (size_t i=0; i<X_enact.size(); ++i) {
        if (roles[i]==Role::Fascist || roles[i]==Role::Hitler) {
            X_F.push_back(X_enact[i]); Y_F.push_back(Y_enact[i]);
        } else {
            X_L.push_back(X_enact[i]); Y_L.push_back(Y_enact[i]);
        }
    }
    int dim = X_enact.empty()? 0 : X_enact[0].size();
    LR_w_enact_F.assign(dim,0.0); LR_b_enact_F=0.0;
    LR_w_enact_L.assign(dim,0.0); LR_b_enact_L=0.0;
    if (!X_F.empty()) trainLogRegSGD(X_F, Y_F, LR_w_enact_F, LR_b_enact_F, lr, epochs);
    if (!X_L.empty()) trainLogRegSGD(X_L, Y_L, LR_w_enact_L, LR_b_enact_L, lr, epochs);
}

GameState determinize(const GameState &rootSt, const BeliefState &B, int myIdx, std::mt19937 &rng) {
	GameState st = rootSt;
	st.rng = rng;
	std::vector<int> unk;
	for (int i = 0; i < 5; ++i) if (i != myIdx) unk.push_back(i);
	struct Assign {
		int hitler, fascist;
	};
	std::vector<Assign> assigns;
	for (int h = 0; h < (int)unk.size(); ++h)
		for (int f = 0; f < (int)unk.size(); ++f) if (f != h)
				assigns.push_back({unk[h], unk[f]});
	std::vector<double> weights(assigns.size());
	double total = 0;
	for (size_t k = 0; k < assigns.size(); ++k) {
		double wgt = 1;
		for (int i : unk) {
			if (i == assigns[k].hitler || i == assigns[k].fascist) wgt *= B.pF[i];
			else wgt *= (1 - B.pF[i]);
		}
		weights[k] = wgt;
		total += wgt;
	}
	std::uniform_real_distribution<double> dist(0.0, total);
	double draw = dist(rng), acc = 0;
	size_t chosen = 0;
	for (size_t k = 0; k < weights.size(); ++k) {
		acc += weights[k];
		if (draw <= acc) {
			chosen = k;
			break;
		}
	}
	for (int i : unk) st.roles[i] = Role::Liberal;
	st.roles[assigns[chosen].hitler]  = Role::Hitler;
	st.roles[assigns[chosen].fascist] = Role::Fascist;
	return st;
}

struct ISMCTS {
	int myPlayer;
	double C;
	std::mt19937 rng;

	struct Node {
		int visits;
		double wins;
		Action action;
		Node *parent;
		std::vector<std::unique_ptr<Node>> children;
		std::vector<Action> untried;
		Node(const Action &a, Node *p) : visits(0), wins(0), action(a), parent(p) {}
	};

	std::unique_ptr<Node> root;

	ISMCTS(int me, double c = 1.414) : myPlayer(me), C(c), rng(std::random_device{}()) {
		root = std::make_unique<Node>(Action(), nullptr);
	}

	double uctScore(Node *n) const {
		if (n->visits == 0) return std::numeric_limits<double>::infinity();
		return (n->wins / n->visits) + C * std::sqrt(std::log(n->parent->visits) / n->visits);
	}

	Action run(const GameState &rootSt, const BeliefState &rootB, int iterations) {
		root = std::make_unique<Node>(Action(), nullptr);
		root->untried = rootSt.getLegalActions();

		for (int it = 0; it < iterations; ++it) {
			GameState det = determinize(rootSt, rootB, myPlayer, rng);

			Node *node = root.get();
			GameState state = det;
			while (node->untried.empty() && !node->children.empty()) {
				node = std::max_element(node->children.begin(), node->children.end(),
				[&](auto &a, auto &b) {
					return uctScore(a.get()) < uctScore(b.get());
				})->get();
				state.apply(node->action);
			}
			if (!node->untried.empty()) {
				Action a = node->untried.back();
				node->untried.pop_back();
				state.apply(a);
				auto child = std::make_unique<Node>(a, node);
				child->untried = state.getLegalActions();
				node->children.push_back(std::move(child));
				node = node->children.back().get();
			}

			GameState sim = state;
			int result;
			while (!sim.isTerminal()) {
				auto acts = sim.getLegalActions();
				if (!acts.empty() && acts[0].type == ActionType::Vote) {
					for (int voter = 0; voter < 5; ++voter) {
						if (!sim.alive[voter]) continue;
						auto phi   = extractFeatures(sim, voter, sim.roles[voter]);
						double pYes = computeVoteYesProb(phi);
						bool vote  = std::bernoulli_distribution(pYes)(sim.rng);
						sim.apply(Action{ ActionType::Vote, voter, -1, vote, -1 });
					}
				} else {
					std::uniform_int_distribution<int> d(0, (int)acts.size()-1);
					sim.apply(acts[d(sim.rng)]);
				}
			}
			result = sim.getWinner();

			double perspective = (rootB.pF[myPlayer] < 0.5 ? +1.0 : -1.0);
			for (Node *n = node; n != nullptr; n = n->parent) {
				n->visits++;
				n->wins += result * perspective;
			}
		}

		Node *best = nullptr;
		int bestVisits = -1;
		for (auto &ch : root->children) {
			if (ch->visits > bestVisits) {
				bestVisits = ch->visits;
				best = ch.get();
			}
		}
		return best ? best->action : Action();
	}
};

void initBeliefs(const GameState &gs, BeliefState &bs, int me) {
	for(int j=0; j<5; ++j) {
		if(j==me) {
			bs.pF[j]=(gs.roles[j]==Role::Liberal?0.0:1.0);
		} else if(gs.roles[me]!=Role::Liberal) {
			bs.pF[j]=(gs.roles[j]!=Role::Liberal?1.0:0.0);
		} else {
			bs.pF[j]=0.5;
		}
	}
}

void selfPlayGen(int games, int myPlayer,
                 std::vector<std::vector<double>>& X_vote, std::vector<int>& Y_vote,
                 std::vector<std::vector<double>>& X_enact, std::vector<int>& Y_enact,
                 std::vector<Role>& recordedRoles) {
	ISMCTS mcts(myPlayer);
	std::mt19937 rng(std::random_device{}());
	int width=50;
	for(int g=0; g<games; ++g) {
		GameState gs;
		BeliefState bs;
		initBeliefs(gs, bs, myPlayer);
		while(!gs.isTerminal()) {
			GameState before=gs;
			auto leg=gs.getLegalActions();
			int actor=leg.front().actor;
			Action a;
			if(actor==myPlayer) {
				a=mcts.run(gs,bs,100);
			} else {
				std::vector<Action> acts;
				for(auto &ac:leg) if(ac.actor==actor) acts.push_back(ac);
				if(!acts.empty() && acts[0].type==ActionType::Vote) {
					auto phi=extractFeatures(gs,actor,gs.roles[actor]);
					double p=computeVoteYesProb(phi);
					std::bernoulli_distribution bd(p);
					bool yes=bd(rng);
					for(auto &ac:acts) if(ac.voteYes==yes) {
							a=ac;
							break;
						}
				} else {
					std::uniform_int_distribution<int> ud(0,(int)leg.size()-1);
					a=leg[ud(rng)];
				}
			}
			if(actor==myPlayer) {
				if(a.type==ActionType::Vote) {
					auto phi=extractFeatures(before,actor,gs.roles[actor]);
					X_vote.push_back(phi);
					Y_vote.push_back(a.voteYes?1:0);
				} else if(a.type==ActionType::Enact) {
					auto phi=extractEnactFeatures(before,actor);
                     X_enact.push_back(phi);
                     Policy chosen = before.drawBuf[a.index];
                    Y_enact.push_back(chosen==Policy::Fascist ? 1 : 0);
                    recordedRoles.push_back(gs.roles[actor]);
				}
			}
			if(a.type==ActionType::Vote) updateBelief(bs,before,a);
			gs.apply(a);
		}
		float frac=float(g+1)/games;
		int pos=int(width*frac);
		std::cout<<"\r[Self-play] [";
		for(int k=0; k<width; ++k) std::cout<<(k<pos?'=':' ');
		std::cout<<"] "<<int(frac*100)<<"%";
	}
	std::cout<<"\n";
}

int main() {
    std::mt19937 rng(std::random_device{}());
    int voteDim = extractFeatures(GameState(), 0, Role::Liberal).size();
    int enactDim = extractEnactFeatures(GameState(), 0).size();

    LR_w_yes.assign(voteDim, 0.0);
    LR_b_yes = 0.0;
    loadWeights("weights.txt");

    LR_w_enact_F.assign(enactDim, 0.0);
    LR_b_enact_F = 0.0;
    LR_w_enact_L.assign(enactDim, 0.0);
    LR_b_enact_L = 0.0;
    loadEnactWeights("weights_enact_F.txt", "weights_enact_L.txt");

    std::vector<std::vector<double>> X_vote;
    std::vector<int>                Y_vote;
    std::vector<std::vector<double>> X_enact;
    std::vector<int>                Y_enact;
    std::vector<Role>               recordedRoles;

    int rounds = 5, gamesPerRound = 200, epochs = 5;
    double lr_rate = 0.01;
    int myIndex = 0;

    for (int r = 0; r < rounds; ++r) {
        X_vote.clear(); Y_vote.clear();
        X_enact.clear(); Y_enact.clear(); recordedRoles.clear();

        selfPlayGen(gamesPerRound, myIndex, X_vote, Y_vote, X_enact, Y_enact, recordedRoles);

        trainLogRegSGD(X_vote, Y_vote, LR_w_yes, LR_b_yes, lr_rate, epochs);
        saveWeights("weights.txt");

        trainEnactModels(X_enact, Y_enact, recordedRoles, lr_rate, epochs);
        saveEnactWeights(
            "weights_enact_F.txt", LR_b_enact_F, LR_w_enact_F,
            "weights_enact_L.txt", LR_b_enact_L, LR_w_enact_L
        );
    }

    int liberalWins = 0;
    ISMCTS tester(myIndex);
    const int testGames = 1000;
    for (int g = 0; g < testGames; ++g) {
        GameState gs;
        BeliefState bs;
        initBeliefs(gs, bs, myIndex);
        while (!gs.isTerminal()) {
            auto leg = gs.getLegalActions();
            int actor = leg.front().actor;
            Action a;
            if (actor == myIndex) {
                a = tester.run(gs, bs, 500);
            } else {
                std::vector<Action> acts;
                for (auto &ac : leg) if (ac.actor == actor) acts.push_back(ac);
                if (!acts.empty() && acts[0].type == ActionType::Vote) {
                    auto phi = extractFeatures(gs, actor, gs.roles[actor]);
                    std::bernoulli_distribution bd(sigmoid(LR_b_yes + std::inner_product(LR_w_yes.begin(), LR_w_yes.end(), phi.begin(), 0.0)));
                    bool yes = bd(rng);
                    for (auto &ac : acts) if (ac.voteYes == yes) { a = ac; break; }
                } else {
                    std::uniform_int_distribution<int> ud(0, (int)acts.size()-1);
                    a = acts.empty() ? Action() : acts[ud(rng)];
                }
            }
            if (a.type == ActionType::Vote) updateBelief(bs, gs, a);
            gs.apply(a);
        }
        if (gs.getWinner() > 0) ++liberalWins;
    }
    std::cout << "Liberal wins: " << liberalWins << "/" << testGames << std::endl;
    return 0;
}
