#pragma once

#include "Types.hpp"
#include "GameState.hpp"
#include <random>

namespace secret_hitler
{
  class GreedyBot
  {
  private:
    int idx;

  public:
    GreedyBot(int index) : idx(index) {}

    Action act(GameState &gs)
    {
      auto acts = gs.getLegalActions();
      auto drawBuf = gs.getDrawBuf();
      auto roles = gs.getRoles();
      Role myRole = roles[idx];

      ActionType type = acts[0].type;
      switch (type)
      {
      case ActionType::Vote:
      {
        int pres = (4 + gs.getNextPresident()) % 5;
        int nom = gs.getNominee();
        bool voteYes;

        if (myRole == Role::Fascist)
        {
          voteYes = (roles[pres] != Role::Liberal || roles[nom] != Role::Liberal);

          for (auto &ac : acts)
            if (ac.type == ActionType::Vote && ac.voteYes == voteYes)
              return ac;
        }
        else
        {
          static thread_local std::mt19937 rng{std::random_device{}()};
          std::uniform_int_distribution<int> ud(0, (int)acts.size() - 1);
          return acts[ud(rng)];
        }
        break;
      }

      case ActionType::Enact:
      {
        Policy want = (myRole == Role::Liberal
                           ? Policy::Liberal
                           : Policy::Fascist);
        for (auto &ac : acts)
          if (drawBuf[ac.index] == want)
            return ac;
        return acts[0];
        break;
      }

      case ActionType::DrawDiscard:
      {
        Policy avoid = (myRole == Role::Liberal
                            ? Policy::Fascist
                            : Policy::Liberal);
        for (auto &ac : acts)
          if (drawBuf[ac.index] == avoid)
            return ac;
        return acts[0];
        break;
      }

      default:
        break;
      }

      static thread_local std::mt19937 rng{std::random_device{}()};
      std::uniform_int_distribution<int> ud(0, (int)acts.size() - 1);
      return acts[ud(rng)];
    }
  };

  class RandomBot
  {
  private:
    int idx;
  public:
    RandomBot(int index) : idx(index) {}
    Action act(GameState &gs)
    {
      auto acts = gs.getLegalActions();
      std::uniform_int_distribution<int> ud(0, acts.size() - 1);
      static thread_local std::mt19937 rng{std::random_device{}()};
      return acts[ud(rng)];
    }
  };

}
