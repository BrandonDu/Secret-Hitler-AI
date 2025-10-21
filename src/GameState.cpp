#include "GameState.hpp"
#include "Types.hpp"
#include <numeric>

namespace secret_hitler
{

    GameState::GameState(std::mt19937 &rng)
    {
        std::array<Role, 5> r = {Role::Liberal, Role::Liberal, Role::Liberal, Role::Hitler, Role::Fascist};
        std::shuffle(r.begin(), r.end(), rng);
        m_roles = r;

        std::vector<Policy> tmp;
        tmp.insert(tmp.end(), 6, Policy::Liberal);
        tmp.insert(tmp.end(), 11, Policy::Fascist);

        std::shuffle(tmp.begin(), tmp.end(), rng);
        m_deck = std::vector<Policy>(tmp.begin(), tmp.end());

        m_alive.fill(true);
    }

    bool GameState::isTerminal() const
    {
        if (m_specialWin != SpecialWin::None)
            return true;
        if (m_enactedLiberal >= 5)
            return true;
        if (m_enactedFascist >= 6)
            return true;
        int aliveL = 0, aliveF = 0;
        for (int i = 0; i < 5; ++i)
        {
            if (!m_alive[i])
                continue;
            if (m_roles[i] == Role::Liberal)
                ++aliveL;
            else
                ++aliveF;
        }
        if (aliveF > aliveL)
            return true;

        return false;
    }

    int GameState::getWinner() const
    {
        if (m_specialWin == SpecialWin::Liberal)
            return +1;
        if (m_specialWin == SpecialWin::Fascist)
            return -1;
        if (m_enactedFascist >= 6)
            return -1;
        if (m_enactedLiberal >= 5)
            return +1;
        int aliveL = 0, aliveF = 0;
        for (int i = 0; i < 5; ++i)
        {
            if (!m_alive[i])
                continue;
            if (m_roles[i] == Role::Liberal)
                ++aliveL;
            else
                ++aliveF;
        }
        if (aliveF > aliveL)
            return -1;
        return 0;
    }

    void GameState::replenishDeck(std::mt19937 &rng)
    {
        int libs = 6 - m_enactedLiberal;
        int facs = 11 - m_enactedFascist;
        std::vector<Policy> pool;
        pool.insert(pool.end(), libs, Policy::Liberal);
        pool.insert(pool.end(), facs, Policy::Fascist);
        std::shuffle(pool.begin(), pool.end(), rng);
        m_deck = pool;
    }

    void GameState::startDrawing(std::mt19937 &rng)
    {
        if (m_deck.size() < 3)
            replenishDeck(rng);
        for (int i = 0; i < 3; ++i)
        {
            m_drawBuf[i] = m_deck.back();
            m_deck.pop_back();
        }
    }

    std::vector<Action> GameState::getLegalActions() const
    {
        std::vector<Action> out;
        int actor;
        switch (m_phase)
        {
        case Nomination:
        {
            actor = m_nextPresident;
            std::vector<int> allCand;
            for (int i = 0; i < 5; ++i)
            {
                if (m_alive[i] && i != actor)
                    allCand.push_back(i);
            }
            std::vector<int> filtered = allCand;
            if (m_lastChancellor >= 0)
            {
                filtered.erase(
                    std::remove(filtered.begin(), filtered.end(), m_lastChancellor),
                    filtered.end());
            }
            if (m_lastPresident >= 0)
            {
                filtered.erase(
                    std::remove(filtered.begin(), filtered.end(), m_lastPresident),
                    filtered.end());
            }
            const auto &cand = filtered.empty() ? allCand : filtered;
            for (int tgt : cand)
            {
                out.push_back({ActionType::Nominate, actor, tgt, false, -1});
            }
            break;
        }
        case Voting:
            for (int i = 0; i < 5; ++i)
            {
                if (!m_alive[i])
                    continue;
                out.push_back({ActionType::Vote, i, -1, true, -1});
                out.push_back({ActionType::Vote, i, -1, false, -1});
            }
            break;
        case Drawing:
        {
            actor = m_nextPresident;
            for (int idx = 0; idx < 3; ++idx)
                out.push_back({ActionType::DrawDiscard, actor, -1, false, idx});
            break;
        }
        case Enacting:
        {
            actor = m_nominee;
            for (int idx = 0; idx < 2; ++idx)
                out.push_back({ActionType::Enact, actor, -1, false, idx});
            if (m_enactedFascist >= 5 && !m_offeredVeto)
                out.push_back({ActionType::Veto, actor, -1, false, -1});
            break;
        }
        case VetoVoting:
            actor = m_nextPresident;
            out.push_back({ActionType::Vote, actor, -1, true, -1});
            out.push_back({ActionType::Vote, actor, -1, false, -1});
            break;
        case Execution:
            actor = m_lastPresident;
            for (int i = 0; i < 5; ++i)
                if (m_alive[i] && i != actor)
                    out.push_back({ActionType::Execute, actor, i, false, -1});
            break;
        }
        return out;
    }

    void GameState::apply(const Action &a, std::mt19937 &rng)
    {
        auto findNextAlive = [&](int cur)
        {
            for (int d = 1; d <= 5; ++d)
            {
                int cand = (cur + d) % 5;
                if (m_alive[cand])
                    return cand;
            }
            return cur;
        };

        switch (m_phase)
        {
        case Phase::Nomination:
            if (a.type == ActionType::Nominate)
            {
                m_lastPresident = m_nextPresident;
                m_nominee = a.target;
                m_phase = Phase::Voting;
                m_votes.clear();
            }
            break;

        case Phase::Voting:
            if (a.type == ActionType::Vote)
            {
                m_votes.emplace_back(a.actor, a.voteYes);
                int aliveCount = std::count(m_alive.begin(), m_alive.end(), true);
                if ((int)m_votes.size() == aliveCount)
                {
                    int yes = 0;
                    for (auto &v : m_votes)
                        if (v.second)
                            ++yes;

                    if (yes >= (double)(aliveCount + 1) / 2)
                    {
                        m_lastChancellor = m_nominee;
                        if (m_enactedFascist >= 3 && m_roles[m_nominee] == Role::Hitler)
                        {
                            m_specialWin = SpecialWin::Fascist;
                            m_phase = Phase::Nomination;
                        }
                        else
                        {
                            m_phase = Phase::Drawing;
                            startDrawing(rng);
                            m_chaosTrack = 0;
                        }
                    }
                    else
                    {
                        m_lastPresident = m_nextPresident;
                        ++m_chaosTrack;
                        m_nextPresident = findNextAlive(m_nextPresident);
                        m_phase = Phase::Nomination;

                        if (m_chaosTrack >= 5)
                        {
                            if (m_deck.empty())
                                replenishDeck(rng);
                            Policy p = m_deck.back();
                            m_deck.pop_back();
                            if (p == Policy::Liberal)
                                ++m_enactedLiberal;
                            else
                                ++m_enactedFascist;
                            m_chaosTrack = 0;
                        }
                    }

                    m_votes.clear();
                }
            }
            break;

        case Phase::Drawing:
            if (a.type == ActionType::DrawDiscard)
            {
                std::vector<Policy> remain;
                for (int i = 0; i < 3; ++i)
                    if (i != a.index)
                        remain.push_back(m_drawBuf[i]);
                m_drawBuf[0] = remain[0];
                m_drawBuf[1] = remain[1];
                m_phase = Phase::Enacting;
            }
            break;

        case Phase::Enacting:
            if (a.type == ActionType::Enact)
            {
                Policy e = m_drawBuf[a.index];
                if (e == Policy::Liberal)
                    ++m_enactedLiberal;
                else
                    ++m_enactedFascist;
                m_lastPresident = m_nextPresident;

                if (e == Policy::Fascist && (m_enactedFascist == 4 || m_enactedFascist == 5))
                {
                    m_phase = Phase::Execution;
                }
                else
                {
                    m_nextPresident = findNextAlive(m_nextPresident);
                    m_phase = Phase::Nomination;
                }
            }
            else if (a.type == ActionType::Veto)
            {
                m_phase = Phase::VetoVoting;
                m_votes.clear();
            }
            m_offeredVeto = false;
            break;

        case Phase::VetoVoting:
            if (a.type == ActionType::Vote && a.actor == m_nextPresident)
            {
                if (a.voteYes)
                {
                    ++m_chaosTrack;
                    if (m_chaosTrack >= 5)
                    {
                        if (m_deck.empty())
                            replenishDeck(rng);
                        Policy p = m_deck.back();
                        m_deck.pop_back();
                        if (p == Policy::Liberal)
                            ++m_enactedLiberal;
                        else
                            ++m_enactedFascist;
                        m_chaosTrack = 0;
                    }
                    m_lastPresident = m_nextPresident;
                    m_nextPresident = findNextAlive(m_nextPresident);
                    m_phase = Phase::Nomination;
                }
                else
                {
                    m_offeredVeto = true;
                    m_phase = Phase::Enacting;
                }
            }
            break;

        case Phase::Execution:
            if (a.type == ActionType::Execute)
            {
                m_alive[a.target] = false;
                if (m_roles[a.target] == Role::Hitler)
                    m_specialWin = SpecialWin::Liberal;
                m_nextPresident = findNextAlive(m_lastPresident);
                m_phase = Phase::Nomination;
            }
            break;
        }
    }

}