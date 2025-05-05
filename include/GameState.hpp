#pragma once

#include <Types.hpp>
#include <vector>
#include <random>

namespace secret_hitler
{

    class GameState
    {
    private:
        enum Phase
        {
            Nomination,
            Voting,
            Drawing,
            Enacting,
            VetoVoting,
            Execution
        } phase;
        enum SpecialWin
        {
            None,
            Liberal,
            Fascist
        };

        std::vector<Policy> m_deck;

        int m_enactedLiberal = 0;
        int m_enactedFascist = 0;
        int m_chaosTrack = 0;
        int m_nextPresident = 0;
        int m_lastChancellor = -1;
        int m_lastPresident = -1;
        int m_nominee = -1;

        std::array<Role, 5> m_roles;
        std::array<bool, 5> m_alive;

        SpecialWin m_specialWin = SpecialWin::None;
        bool m_offeredVeto = false;
        Phase m_phase = Phase::Nomination;

        std::array<Policy, 3> m_drawBuf;
        std::vector<std::pair<int, bool>> m_votes;

    public:
        explicit GameState(std::mt19937 &rng);
        bool isTerminal() const;
        int getWinner() const;

        void replenishDeck(std::mt19937 &rng);
        void startDrawing(std::mt19937 &rng);

        int getEnactedLiberal() const { return m_enactedLiberal; }
        int getEnactedFascist() const { return m_enactedFascist; }
        int getChaosTrack() const { return m_chaosTrack; }
        Phase getPhase() const { return m_phase; }
        const std::array<Policy, 3> &getDrawBuf() const { return m_drawBuf; }
        const std::array<Role, 5> &getRoles() const { return m_roles; }
        const std::array<bool, 5> &isAlive() const { return m_alive; }

        std::vector<Action> getLegalActions() const;
        void apply(const Action &a, std::mt19937 &rng);
    };

}