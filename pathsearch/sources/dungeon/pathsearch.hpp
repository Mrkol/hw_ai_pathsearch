#pragma once

#include "dungeon.hpp"
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <experimental/mdarray>
#include <experimental/generator>
#include <function2/function2.hpp>


namespace dungeon
{

constexpr float INF = 1e6;

using Dists = std::experimental::mdarray<float, std::experimental::extents<int, std::dynamic_extent, std::dynamic_extent>>;
using DistsView = std::experimental::mdspan<float, std::experimental::extents<int, std::dynamic_extent, std::dynamic_extent>>;

struct SearchResult
{
  std::vector<glm::ivec2> path;
  Dists dists;
  float dist{};
};

SearchResult aStar(DungeonView dungeon, glm::ivec2 start, glm::ivec2 finish, float eps);

SearchResult smaStar(DungeonView dungeon, glm::ivec2 start, glm::ivec2 finish, float eps);

std::experimental::generator<SearchResult> araStar(DungeonView dungeon, glm::ivec2 start, glm::ivec2 finish, float eps);


struct Portal
{
  glm::ivec2 topLeft;
  glm::ivec2 bottomRight;

  glm::vec2 midpoint() const { return (glm::vec2{topLeft} + glm::vec2{bottomRight}) / 2.f; }
  bool contains(glm::ivec2 v) const { return glm::min(v, topLeft) == topLeft && glm::max(v + 1, bottomRight) == bottomRight; }

  struct Adjacent
  {
    // Starts from SOME point inside the current portal
    std::vector<glm::ivec2> path;
    float dist;
  };

  std::unordered_map<std::size_t, Adjacent> adjacent;
};

struct HierarchicalSearchData
{
  int cellSize{0};
  std::vector<Portal> portals;
  std::unordered_multimap<glm::ivec2, std::size_t> cellToPortalList;
};

HierarchicalSearchData buildHierarchy(DungeonView dungeon, int cellSize);

// Only returns a path -- no dists
SearchResult hierarchicalSearch(DungeonView dungeon, const HierarchicalSearchData& data, glm::ivec2 start, glm::ivec2 finish);


}
