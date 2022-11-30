#pragma once

#include <glm/glm.hpp>

#include "Allegro.hpp"
#include "Game.hpp"


class Application
  : public Allegro<Application>
  , public Game<Application>
{
public:
  Application(int, char**)
  {
  }

  void drawGui()
  {
    Game::drawGui();
  }

  glm::vec2 worldToScreen(glm::vec2 v)
  {
    float scale = camScale * 0.05f * (kWidth + kHeight) / 2.f;
    return glm::vec2{kWidth, kHeight}/2.f + (v - camPos) * scale;
  };

  glm::vec2 screenToWorld(glm::vec2 v)
  {
    float scale = camScale * 0.05f * (kWidth + kHeight) / 2.f;
    return  (v - glm::vec2{kWidth, kHeight}/2.f)/scale + camPos;
  }

  int run()
  {
    while (!exit_)
    {
      Allegro::poll();
      Allegro::tryRedraw();
    }

    return 0;
  }

  void close()
  {
    exit_ = true;
  }

public:
  float camScale = 1.f;
  glm::vec2 camPos{};

private:
  bool exit_ = false;
};
