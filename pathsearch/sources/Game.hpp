#pragma once

#include <array>

#include <glm/glm.hpp>
#include <function2/function2.hpp>
#include <limits>
#include <optional>
#include <spdlog/fmt/fmt.h>
#include <allegro5/keycodes.h>
#include <allegro5/allegro5.h>

#include "allegro5/allegro_font.h"
#include "allegro5/allegro_primitives.h"
#include "allegro5/color.h"
#include "allegro5/mouse.h"
#include "dungeon/pathsearch.hpp"
#include "glm/geometric.hpp"
#include "imgui.h"
#include "util.hpp"

#include "dungeon/dungeon.hpp"
#include "dungeon/dungeonGenerator.hpp"
#include "dungeon/dungeonUtils.hpp"


template<class Derived>
class Game
{
 public:
  Game()
    : dungeon_{dungeon::make_dungeon(50, 50)}
  {
    dungeon::gen_drunk_dungeon(dungeon_.view);

    searchStart_ = dungeon::find_walkable_tile(dungeon_.view);
    searchEnd_ = dungeon::find_walkable_tile(dungeon_.view);

    restartSearch();
  }

  void mouse(float x, float y)
  {
    auto prev = self().screenToWorld(mousePosition_);
    mousePosition_ = {x, y};
    auto curr = self().screenToWorld(mousePosition_);
    if (dragging_)
      self().camPos += prev - curr;
  }

  void wheel(float z)
  {
    auto prev = self().screenToWorld(mousePosition_);
    self().camScale = std::exp(z / 10.f);
    auto curr = self().screenToWorld(mousePosition_);
    self().camPos += prev - curr;
  }

  void drawGui()
  {
    ImGui::Begin("Kek");
    ImGui::Checkbox("Additional debug info", &additionalDebugInfo_);
    if (ImGui::InputFloat("Eps", &searchEps_))
      restartSearch();

    ImGui::BeginDisabled(searchIterator_ == searcher_.end());
    if (ImGui::Button("Next Iteration"))
      updateSearch();
    ImGui::EndDisabled();

    ImGui::End();
  }

  void mouseDown(unsigned int button)
  {
    switch (button)
    {
      case 3:
        dragging_ = true;
        break;

      default:
        break;
    }
  }

  void mouseUp(unsigned int button)
  {
    switch (button)
    {
      case 1:
        searchStart_ = glm::ivec2(self().screenToWorld(mousePosition_));
        restartSearch();
        break;

      case 2:
        searchEnd_ = glm::ivec2(self().screenToWorld(mousePosition_));
        restartSearch();
        break;

      case 3:
        dragging_ = false;
        break;

      default:
        break;
    }
  }

  void updateSearch()
  {
    if (searchIterator_ != searcher_.end())
    {
      ++searchIterator_;
      if (searchIterator_ != searcher_.end())
        searchResult_ = std::move(*searchIterator_);
    }
  }

  void restartSearch()
  {
    searcher_ = dungeon::araStar(dungeon_.view, searchStart_, searchEnd_, 2.5f);
    searchIterator_ = searcher_.begin();
    if (searchIterator_ != searcher_.end())
      searchResult_ = std::move(*searchIterator_);
  }

  void draw()
  {
    static auto wallSprite = self().loadSprite(PROJECT_SOURCE_DIR "/pathsearch/resources/wall_mid.png");
    static auto floorSprite = self().loadSprite(PROJECT_SOURCE_DIR "/pathsearch/resources/floor_1.png");
    static auto waterSprite = self().loadSprite(PROJECT_SOURCE_DIR "/pathsearch/resources/water.png");

    auto bitmapFor = [&](dungeon::Tile tile)
      {
        switch (tile)
        {
          case dungeon::Tile::Floor:
            return self().getSpriteBitmap(floorSprite);

          case dungeon::Tile::Wall:
            return self().getSpriteBitmap(wallSprite);

          case dungeon::Tile::Water:
            return self().getSpriteBitmap(waterSprite);
        }
      };

    for (int y = 0; y < dungeon_.view.extent(0); ++y)
      for (int x = 0; x < dungeon_.view.extent(1); ++x)
      {
        auto min = self().worldToScreen({x, y});
        auto max = self().worldToScreen(glm::ivec2{x, y} + glm::ivec2{1, 1});

        auto bmp = bitmapFor(dungeon_.view(y, x));
        al_draw_scaled_bitmap(
          bmp,
          0, 0, al_get_bitmap_width(bmp), al_get_bitmap_height(bmp),
          min.x, min.y, max.x - min.x, max.y - min.y, ALLEGRO_FLIP_VERTICAL);

        if (searchResult_.dists(y, x) != dungeon::INF)
        {
          al_draw_filled_rectangle(min.x, min.y, max.x, max.y, al_map_rgba(255, 255, 0, 32));
          if (additionalDebugInfo_)
            al_draw_text(self().getFont(), al_map_rgba(255, 255, 225, 255), min.x, min.y, {},
              fmt::format("{}", searchResult_.dists(y, x)).c_str());
        }
      }

    for (auto v : searchResult_.path)
    {
      auto min = self().worldToScreen(v);
      auto max = self().worldToScreen(v + glm::ivec2{1, 1});

      al_draw_filled_rectangle(min.x, min.y, max.x, max.y, al_map_rgba(0, 255, 0, 32));
    }
  }

 private:
  Derived& self() { return *static_cast<Derived*>(this); }
  const Derived& self() const { return *static_cast<const Derived*>(this); }

 private:
  glm::vec2 mousePosition_{};
  bool dragging_{false};

  float searchEps_{2.5f};

  bool additionalDebugInfo_;

  glm::ivec2 searchStart_;
  glm::ivec2 searchEnd_;

  dungeon::SearchResult searchResult_;
  std::experimental::generator<dungeon::SearchResult>::iterator searchIterator_;
  std::experimental::generator<dungeon::SearchResult> searcher_;

  dungeon::Dungeon dungeon_;
};
