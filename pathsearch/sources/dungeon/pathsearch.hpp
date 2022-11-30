#pragma once

#include "dungeon.hpp"
#include <glm/glm.hpp>
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
};

SearchResult aStar(DungeonView dungeon, glm::ivec2 start, glm::ivec2 finish, float eps);

SearchResult smaStar(DungeonView dungeon, glm::ivec2 start, glm::ivec2 finish, float eps);

std::experimental::generator<SearchResult> araStar(DungeonView dungeon, glm::ivec2 start, glm::ivec2 finish, float eps);

}
