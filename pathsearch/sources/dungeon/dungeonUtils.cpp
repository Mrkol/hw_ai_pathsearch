#include "dungeonUtils.hpp"
#include <vector>
#include <random>


namespace dungeon
{

glm::ivec2 find_walkable_tile(DungeonView view)
{
  static std::default_random_engine engine;

  // prebuild all walkable and get one of them
  std::vector<glm::ivec2> posList;

  for (int y = 0; y < view.extent(0); ++y)
    for (int x = 0; x < view.extent(1); ++x)
      if (view(y, x) == Tile::Floor)
        posList.push_back(glm::ivec2{x, y});

  std::uniform_int_distribution<> distr(0, posList.size() - 1);
  return posList[distr(engine)];
}

bool is_tile_walkable(DungeonView view, glm::ivec2 pos)
{
  if (pos.x < 0 || pos.x >= view.extent(1) ||
      pos.y < 0 || pos.y >= view.extent(0))
    return false;
  return view(pos.y, pos.x) == Tile::Floor;
}

Dungeon make_dungeon(int width, int height)
{
  Dungeon result
    {
      .data = std::vector<Tile>(width * height),
    };
  result.view = DungeonView(result.data.data(), width, height);
  return result;
}

}

