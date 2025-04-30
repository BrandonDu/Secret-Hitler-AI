#include <algorithm>
#include <array>
#include <cmath>
#include <deque>
#include <iostream>
#include <random>
#include <utility>
#include <vector>

enum class Policy {Liberal, Fascist};
enum class Role {Liberal, Fascist, Hitler};
enum class ActionType {Nominate, Vote, DrawDiscard, Enact, Veto};

struct Action {
    ActionType type;
    int actor;
    int target;
    bool vote;
    int discardIdx;
};

struct GameState {
    enum Phase {Nomination, Voting, Drawing, Enacting} phase;


    // vector<Action> history ;
    std::vector<std::pair<int, bool>> votes; 
    int enactedLiberal = 0;
    int enactedFascist = 0;
    int chaosTrack = 0;

    int electedChancellor = -1;
    int lastChancellor = -1; 
    int lastPresident = -1;
    int nextPresident = -1;
    bool electedHitler = false;

    std::array<Policy, 3> drawnCards; 
    // Hidden information
    std::array<Role, 5> roles;
    std::vector<Policy> deck; 

    GameState() {
        std::array<Role, 5> v = {Role::Hitler, Role::Fascist, Role::Liberal, Role::Liberal, Role::Liberal};
        std::shuffle(v.begin(), v.end(), std::mt19937(std::random_device{}()));
        roles = v;

        deck.insert(deck.end(), 6, Policy::Liberal);
        deck.insert(deck.end(), 11, Policy::Fascist);
        std::shuffle(deck.begin(), deck.end(), std::mt19937(std::random_device{}()));
    }
    bool isTerminal() const {return enactedFascist == 6 || enactedLiberal == 5 || electedHitler;}

    int getWinner() const {
        if (enactedLiberal == 5) return 1;
        return -1;
    }
    std::vector<Action> getPossibleActions() const {
        std::vector<Action> possibleActions;
        int actor;
        switch(phase) {
            case Nomination:
                actor = nextPresident;
                for (int i = 0; i < 5; ++i) {
                    if (i == actor || i == lastChancellor || i == lastPresident) {
                        continue;
                    }
                    possibleActions.push_back({ActionType::Nominate, actor, i, false, -1});
                }
                break;
            case Voting:
                for (int i = 0; i < 5; ++i) {
                    possibleActions.push_back({ActionType::Enact, i, -1, true, -1});
                    possibleActions.push_back({ActionType::Enact, i, -1, false, -1});
                }
                break;
            case Drawing:
                actor = nextPresident;
                for (int i = 0; i < 3; ++i) {
                    possibleActions.push_back({ActionType::DrawDiscard, actor, -1, false, i});
                }
                break;
            case Enacting:
                actor = electedChancellor;
                for (int i = 0; i < 2; ++i) {
                    possibleActions.push_back({ActionType::Enact, actor, -1, false, i});
                }
                break;
        }
        return possibleActions;
    }

    void performAction(const Action &a) {
        switch(a.type) {
            case ActionType::Nominate:
                electedChancellor = a.target;
                phase = Voting;
                votes.clear();
                break;
            case ActionType::Vote: {
                                       votes.emplace_back(a.actor, a.vote);
                                       if (votes.size() < 5) break;
                                       int yesCount = 0;
                                       for (const auto& v: votes) {
                                           if (v.second) ++yesCount;
                                       }
                                       if (yesCount >= 3) {
                                           if (enactedFascist >= 3 && roles[electedChancellor] == Role::Hitler) {
                                               electedHitler = true;
                                           }
                                           else {
                                               phase = Drawing;
                                               for (int i = 0; i < 3; ++i) {
                                                   drawnCards[i] = deck.back();
                                                   deck.pop_back();
                                               }

                                               chaosTrack = 0;
                                               lastChancellor = electedChancellor;
                                               lastPresident = nextPresident;
                                           }
                                       }
                                       else {
                                           ++chaosTrack;
                                           if (chaosTrack == 5) {
                                               Policy p = deck.back();
                                               deck.pop_back();
                                               if (p == Policy::Liberal) ++enactedLiberal;
                                               else ++enactedFascist;
                                               chaosTrack = 0;
                                               nextPresident = (nextPresident + 1) % 5;
                                               phase = Nomination;
                                           }
                                       }
                                       break;
                                   }
            case ActionType::DrawDiscard:
                                   if (a.discardIdx == 0 || a.discardIdx == 1) {
                                       drawnCards[a.discardIdx] = drawnCards[2];
                                   }
                                   phase = Enacting;
                                   break;
            case ActionType::Enact:
                                   {
                                       Policy enact = drawnCards[1 - a.discardIdx];
                                       if (enact == Policy::Liberal) ++enactedLiberal;
                                       else ++enactedFascist;

                                       if (deck.size() < 3) {
                                           int remainingLiberals = 6 - enactedLiberal;
                                           int remainingFascists = 6 - enactedFascist;
                                           std::vector<Policy> pool;
                                           pool.reserve(remainingLiberals + remainingFascists);
                                           pool.insert(pool.end(), remainingLiberals, Policy::Liberal);
                                           pool.insert(pool.end(), remainingFascists, Policy::Fascist);
                                           std::mt19937 rng_shuf(std::random_device{}());
                                           std::shuffle(pool.begin(), pool.end(), std::mt19937(std::random_device{}()));
                                       }

                                       nextPresident = (nextPresident + 1) % 5;
                                       phase = Nomination;
                                   }
            default: // TODO Veto case
                                   break;
        }
    }

};

int main() {
    return 0;
}
