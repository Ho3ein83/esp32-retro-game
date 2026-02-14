#pragma once

class Scene {
public:
  virtual void enter() = 0;
  virtual void update() = 0;
  virtual void render() = 0;
  virtual void exit() = 0;
  virtual ~Scene() {}
};