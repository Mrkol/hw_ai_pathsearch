#include "pathsearch.hpp"
#include "dungeon/dungeon.hpp"
#include "dungeon/pathsearch.hpp"
#include "../glmFormatter.hpp"

#include <algorithm>
#include <unordered_set>
#include <queue>
#include <map>
#include <glm/gtx/hash.hpp>
#include <spdlog/spdlog.h>
#include <fmt/ranges.h>


namespace dungeon
{

bool inBounds(glm::ivec2 v,
  std::experimental::extents<int, std::experimental::dynamic_extent, std::experimental::dynamic_extent> extents)
{
  return v.x >= 0 && v.y >= 0 && v.x < extents.extent(1) && v.y < extents.extent(0);
}

float ivecDist(glm::ivec2 a, glm::ivec2 b)
{
  return glm::length(glm::vec2{a - b});
}

float weight(DungeonView view, glm::ivec2 a, glm::ivec2 b)
{
  return ivecDist(a, b)
    + (view(a.y, a.x) == Tile::Water || view(b.y, b.x) == Tile::Water ? 5 : 0);
}

auto successorsFor(glm::ivec2 v, DungeonView dungeon)
{
  std::vector<glm::ivec2> result;
  result.reserve(4);
  for (auto offset : std::array{
    glm::ivec2{0, 1}, glm::ivec2{0, -1}, glm::ivec2{1, 0}, glm::ivec2{-1, 0}})
  {
    auto successor = v + offset;
    if (!inBounds(successor, dungeon.extents())
      || dungeon(successor.y, successor.x) == Tile::Wall)
      continue;
    result.push_back(successor);
  }
  return result;
}

static std::vector<glm::ivec2> reconstructPath(DungeonView dungeon, DistsView dists, glm::ivec2 start, glm::ivec2 finish)
{
  std::vector<glm::ivec2> result;

  if (dists(finish.y, finish.x) != INF)
  {
    glm::ivec2 current = finish;
    while (current != start)
    {
      glm::ivec2 best = current;
      for (auto successor : successorsFor(current, dungeon))
      {
        if (dists(successor.y, successor.x) + weight(dungeon, current, successor) == dists(best.y, best.x))
          best = successor;
      }
      result.push_back(current);
      current = best;
    }
    result.push_back(start);
  }

  return result;
}

auto initAStar(DungeonView dungeon, glm::ivec2 start, glm::ivec2 finish, float eps)
{
  using Pair = std::pair<float, glm::ivec2>;

  struct Comp
  {
    bool operator()(const Pair& a, const Pair& b)
      { return a.first > b.first; }
  };

  std::priority_queue<Pair, std::vector<Pair>, Comp> queue(Comp{},
    [&]()
    {
      std::vector<Pair> container;
      container.reserve(dungeon.extent(0) * 4);
      return container;
    }());

  Dists dists{dungeon.extents()};
  std::fill_n(dists.data(),  dists.size(), INF);

  if (inBounds(start, dungeon.extents()))
  {
    queue.push({eps*ivecDist(start, finish), start});
    dists(start.y, start.x) = 0;
  }

  return std::make_pair(std::move(queue), std::move(dists));
}

SearchResult aStar(DungeonView dungeon, glm::ivec2 start, glm::ivec2 finish, float eps)
{
  auto[queue, dists] = initAStar(dungeon, start, finish, eps);

  while (!queue.empty())
  {
    auto current = queue.top().second;
    queue.pop();

    if (current == finish)
      break;

    auto dist = dists(current.y, current.x);

    for (auto successor : successorsFor(current, dungeon))
    {
      float successor_dist = dist + weight(dungeon, current, successor);
      if (successor_dist < dists(successor.y, successor.x))
      {
        dists(successor.y, successor.x) = successor_dist;
        queue.push({successor_dist + eps*ivecDist(successor, finish), successor});
      }
    }
  }

  auto path = reconstructPath(dungeon, {dists.data(), dists.extents()}, start, finish);

  return {std::move(path), std::move(dists)};
}

// SearchResult smaStar(DungeonView dungeon, glm::ivec2 start, glm::ivec2 finish, float eps)
// {
//   struct Key { std::uint32_t depth; float fScore; };
//   struct Value { glm::vec2 node; ; };
//   std::map<Key, Value> queue;

//   if (inBounds(start, dungeon.extents()))
//   {
//     queue.emplace(Key{0, potential(start)}, Value{start, });
//   }

//   while (!queue.empty())
//   {
//     auto[k, v] = *queue.begin();


//     queue.erase(queue.begin());
//   }
// }



std::experimental::generator<SearchResult> araStar(DungeonView dungeon, glm::ivec2 start, glm::ivec2 finish, float eps)
{
  auto[open, dists] = initAStar(dungeon, start, finish, eps);

  const auto fScore = [&eps, finish, &dists = dists](glm::ivec2 v) { return dists(v.y, v.x) + eps*ivecDist(v, finish); };

  for (;;)
  {
    std::unordered_set<glm::ivec2> closed;
    std::unordered_set<glm::ivec2> inconsistent;

    while (!open.empty() && fScore(finish) > open.top().first)
    {
      const auto current = open.top().second;
      open.pop();

      closed.emplace(current);

      for (auto successor : successorsFor(current, dungeon))
      {
        const float succDist = dists(current.y, current.x) + weight(dungeon, current, successor);
        if (dists(successor.y, successor.x) > succDist)
        {
          dists(successor.y, successor.x) = succDist;
          if (!closed.contains(successor))
            open.emplace(fScore(successor), successor);
          else
            inconsistent.emplace(successor);
        }
      }
    }

    {
      float minScore = INF;
      {
        // Could be im plemented in O(1) with hybrid data structures, but I'm too lazy
        auto updateMin = [&minScore, &dists = dists, finish](auto& set)
          {
            for (const auto& v : set)
              minScore = std::min(minScore, dists(v.y, v.x) + ivecDist(v, finish));
          };
        updateMin(closed);
        updateMin(inconsistent);
      }
      float epsPrime = std::min(eps, dists(finish.y, finish.x)/minScore);

      if (epsPrime <= 1)
        break;
    }

    co_yield SearchResult{reconstructPath(dungeon, {dists.data(), dists.extents()}, start, finish), dists};

    eps -= 0.25;
    auto oldOpen = std::move(open);
    while (!oldOpen.empty())
    {
      auto v = oldOpen.top().second;
      open.emplace(fScore(v), v);
      oldOpen.pop();
    }
    for (auto v : inconsistent)
      open.emplace(fScore(v), v);
  }
}

}
