#pragma once
#include "../engine/Scene.h"

class SplashScene : public Scene {
public:
  void enter() override;
  void update() override;
  void render() override;
  void exit() override;
};

extern SplashScene splashScene;