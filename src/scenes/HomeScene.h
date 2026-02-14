#pragma once
#include "../engine/Scene.h"

class HomeScene : public Scene {
public:
  void enter() override;
  void update() override;
  void render() override;
  void exit() override;
};

extern HomeScene homeScene;