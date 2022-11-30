#pragma once
#include <glm/glm.hpp>

#include "dungeon.hpp"


namespace dungeon
{

glm::ivec2 find_walkable_tile(DungeonView view);
bool is_tile_walkable(DungeonView view, glm::ivec2 pos);
Dungeon make_dungeon(int width, int height);

};
