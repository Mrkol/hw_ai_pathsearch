#include "pathsearch.hpp"
#include "assert.hpp"
#include "dungeon/dungeon.hpp"
#include "dungeon/pathsearch.hpp"
#include "../glmFormatter.hpp"

#include <algorithm>
#include <unordered_set>
#include <queue>
#include <map>
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


HierarchicalSearchData buildHierarchy(DungeonView dungeon, int cellSize)
{
  NG_ASSERT(dungeon.extent(0) % cellSize == 0 && dungeon.extent(1) % cellSize == 0);

  HierarchicalSearchData result{.cellSize = cellSize};

  for (int y = 0; y < dungeon.extent(1) / cellSize; ++y)
  {
    for (int x = 0; x < dungeon.extent(0) / cellSize; ++x)
    {
      glm::ivec2 cellStart = cellSize * glm::ivec2{x, y};
      // Find portals on top
      if (y > 0)
      {
        const int yStart = cellStart.y - 1;
        const int yEnd = cellStart.y + 1;

        for (int xStart = cellStart.x; xStart < cellStart.x + cellSize; ++xStart)
        {
          int xEnd = xStart;
          while (xEnd < cellStart.x + cellSize && dungeon(yStart, xEnd) != Tile::Wall && dungeon(yStart + 1, xEnd) != Tile::Wall)
            ++xEnd;

          if (xEnd > xStart)
          {
            result.portals.push_back(Portal{glm::ivec2{xStart, yStart}, glm::ivec2{xEnd, yEnd}, {}});
            result.cellToPortalList.emplace(glm::ivec2{x, y}, result.portals.size() - 1);
            if (y > 0)
              result.cellToPortalList.emplace(glm::ivec2{x, y - 1}, result.portals.size() - 1);
            xStart = xEnd;
          }
        }
      }

      // Find portals to the left
      if (x > 0)
      {
        const int xStart = cellStart.x - 1;
        const int xEnd = cellStart.x + 1;

        for (int yStart = cellStart.y; yStart < cellStart.y + cellSize; ++yStart)
        {
          int yEnd = yStart;
          while (yEnd < cellStart.y + cellSize && dungeon(yEnd, xStart) != Tile::Wall && dungeon(yEnd, xStart + 1) != Tile::Wall)
            ++yEnd;

          if (yEnd > yStart)
          {
            result.portals.push_back(Portal{glm::ivec2{xStart, yStart}, glm::ivec2{xEnd, yEnd}, {}});
            result.cellToPortalList.emplace(glm::ivec2{x, y}, result.portals.size() - 1);
            if (x > 0)
              result.cellToPortalList.emplace(glm::ivec2{x - 1, y}, result.portals.size() - 1);
            yStart = yEnd;
          }
        }
      }
    }
  }

  for (int y = 0; y < dungeon.extent(0) / cellSize; ++y)
  {
    for (int x = 0; x < dungeon.extent(1) / cellSize; ++x)
    {
      glm::ivec2 cellStart = cellSize * glm::ivec2{x, y};

      // Run floyd's algo to find shortest paths between all points

      using Exts = std::experimental::extents<int,
        std::experimental::dynamic_extent, std::experimental::dynamic_extent, std::experimental::dynamic_extent, std::experimental::dynamic_extent>;

      std::experimental::mdarray<float, Exts> dists(Exts{cellSize, cellSize, cellSize, cellSize}, INF);
      std::experimental::mdarray<glm::ivec2, Exts> next(dists.extents());

      // Encode the cell subgraph into dists/next tensor
      for (int y1 = 0; y1 < dists.extent(0); ++y1)
      {
        for (int x1 = 0; x1 < dists.extent(1); ++x1)
        {
          glm::ivec2 v{x1, y1};
          dists(y1, x1, y1, x1) = 0;
          next(y1, x1, y1, x1) = v;

          for (auto w : successorsFor(v + cellStart, dungeon))
          {
            auto w1 = w - cellStart;
            if (w1.x < 0 || w1.y < 0 || w1.x >= cellSize || w1.y >= cellSize)
              continue;
            dists(y1, x1, w1.y, w1.x) = weight(dungeon, v + cellStart, w);
            next(y1, x1, w1.y, w1.x) = w1;
          }
        }
      }

      // Run the optimization procedure
      for (int y1 = 0; y1 < cellSize; ++y1)
        for (int x1 = 0; x1 < cellSize; ++x1)
          for (int y2 = 0; y2 < cellSize; ++y2)
            for (int x2 = 0; x2 < cellSize; ++x2)
              for (int y3 = 0; y3 < cellSize; ++y3)
                for (int x3 = 0; x3 < cellSize; ++x3)
                {
                  // Try to optimize path from 2 to 3 through 1
                  float& oldDist = dists(y2, x2, y3, x3);
                  float newDist = dists(y2, x2, y1, x1) + dists(y1, x1, y3, x3);
                  if (oldDist > newDist)
                  {
                    oldDist = newDist;
                    next(y2, x2, y3, x3) = next(y2, x2, y1, x1);
                  }
                }

      const auto[b, e] = result.cellToPortalList.equal_range(glm::ivec2{x, y});
      for (auto i = b; i != e; ++i)
      {
        for (auto j = b; j != e; ++j)
        {
          if (i == j)
            continue;

          const auto& p1 = result.portals[i->second];
          const auto& p2 = result.portals[j->second];

          const auto p1TopLeft     = glm::min(glm::max(p1.topLeft     - cellStart, glm::ivec2{0}), glm::ivec2{cellSize});
          const auto p1BottomRight = glm::min(glm::max(p1.bottomRight - cellStart, glm::ivec2{0}), glm::ivec2{cellSize});
          const auto p2TopLeft     = glm::min(glm::max(p2.topLeft     - cellStart, glm::ivec2{0}), glm::ivec2{cellSize});
          const auto p2BottomRight = glm::min(glm::max(p2.bottomRight - cellStart, glm::ivec2{0}), glm::ivec2{cellSize});

          glm::ivec2 start = p1TopLeft;
          glm::ivec2 end = p2TopLeft;
          float shortest = INF;
          float longest = 0;
          for (int yStart = p1TopLeft.y; yStart < p1BottomRight.y; ++yStart)
            for (int xStart = p1TopLeft.x; xStart < p1BottomRight.x; ++xStart)
              for (int yEnd = p2TopLeft.y; yEnd < p2BottomRight.y; ++yEnd)
                for (int xEnd = p2TopLeft.x; xEnd < p2BottomRight.x; ++xEnd)
                {
                  float d = dists(yStart, xStart, yEnd, xEnd);
                  if (d < shortest)
                  {
                    shortest = d;
                    start = {xStart, yStart};
                    end = {xEnd, yEnd};
                  }
                  if (d > longest)
                    longest = d;
                }

          if (shortest >= INF)
            continue;

          // record as adjacent

          auto& adj = result.portals[i->second].adjacent[j->second];

          adj.dist = (shortest + longest) / 2.f; // dirty hack

          while (start != end)
          {
            adj.path.push_back(start + cellStart);
            start = next(start.y, start.x, end.y, end.x);
          }
          adj.path.push_back(start + cellStart);
        }
      }
    }
  }

  return result;
}

static std::vector<std::size_t> portalSearch(const HierarchicalSearchData& data, std::size_t start, std::size_t finish)
{
  using Pair = std::pair<float, std::size_t>;

  struct Comp
  {
    bool operator()(const Pair& a, const Pair& b)
      { return a.first > b.first; }
  };


  auto portalDist = [&data](std::size_t a, std::size_t b) { return glm::length(data.portals[a].midpoint() - data.portals[b].midpoint()); };

  std::priority_queue<Pair> queue;
  std::vector<float> dists(data.portals.size(), INF);
  queue.push({portalDist(start, finish), start});
  dists[start] = 0;

  while (!queue.empty())
  {
    auto current = queue.top().second;
    queue.pop();

    if (current == finish)
      break;

    auto dist = dists[current];

    for (const auto&[successor, adj] : data.portals[current].adjacent)
    {
      float successor_dist = dist + adj.dist;
      if (successor_dist < dists[successor])
      {
        dists[successor] = successor_dist;
        queue.push({successor_dist + portalDist(successor, finish), successor});
      }
    }
  }

  std::vector<std::size_t> result;

  if (dists[finish] != INF)
  {
    auto current = finish;

    result.push_back(current);
    while (current != start)
    {
      auto best = current;
      for (const auto&[successor, _] : data.portals[current].adjacent)
        if (dists[successor] < dists[best])
          best = successor;

      if (best == current)
        break;

      current = best;
      result.push_back(current);
    }
  }

  std::reverse(result.begin(), result.end());

  return result;
}

// Walks "straight" to the target. Behaviour undefined if the target is not reachable that way
static void appendStraightPath(DungeonView dungeon, SearchResult& result, glm::ivec2 target)
{
  while (result.path.back() != target)
  {
    auto best = result.path.back();
    for (auto successor : successorsFor(result.path.back(), dungeon))
      if (ivecDist(best, target) > ivecDist(successor, target))
        best = successor;

    if (best == result.path.back())
      break;

    result.path.push_back(best);
  }
}

SearchResult hierarchicalSearch(DungeonView dungeon, const HierarchicalSearchData& data, glm::ivec2 start, glm::ivec2 finish)
{
  // This is hacky. Only clicking on portal tiles is supported.
  // It is not clear how to do the general case:
  // - Check all possible "entrance" and "exit" portals? Too long!
  // - Come to the closest portal? Can be extremely non-optimal!

  constexpr std::size_t NOT_FOUND = static_cast<std::size_t>(-1);

  std::size_t entrance = NOT_FOUND;
  std::size_t exit = NOT_FOUND;
  for (std::size_t i = 0; i < data.portals.size(); ++i)
  {
    if (data.portals[i].contains(start))
      entrance = i;
    if (data.portals[i].contains(finish))
      exit = i;
  }

  SearchResult result;
  if (entrance == NOT_FOUND || exit == NOT_FOUND)
    return result;

  result.path.push_back(start);

  auto portalPath = portalSearch(data, entrance, exit);

  for (std::size_t i = 1; i < portalPath.size(); ++i)
  {
    const auto portalFrom = portalPath[i - 1];
    const auto portalTo = portalPath[i];

    const auto& path = data.portals[portalFrom].adjacent.at(portalTo).path;

    appendStraightPath(dungeon, result, path.front());

    result.path.insert(result.path.cend(), path.begin(), path.end());
  }

  appendStraightPath(dungeon, result, finish);

  return result;
}

}
