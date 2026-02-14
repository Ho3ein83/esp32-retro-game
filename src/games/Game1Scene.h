#pragma once
#include "../engine/Scene.h"

class Game1Scene : public Scene {
public:
  void enter() override;
  void update() override;
  void render() override;
  void exit() override;
};

extern Game1Scene game1Scene;