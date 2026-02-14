#pragma once
#include "Scene.h"

class SceneManager {
private:
  Scene* current = nullptr;
  Scene* next    = nullptr;

public:
  void set(Scene* s) {
    next = s;
  }

  void update() {
    if (next) {
      if (current) current->exit();
      current = next;
      current->enter();
      next = nullptr;
    }

    if (current) current->update();
  }

  void render() {
    if (current) current->render();
  }
};
