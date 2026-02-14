#include <stdint.h>
#include "Game1Scene.h"
#include "esp_random.h"
#include "../engine/Globals.h"
#include "../engine/Input.h"
#include "../scenes/HomeScene.h"
#include "../sprites/girl_walk_right.h"
#include "../sprites/girl_walk_left.h"
#include "../sprites/girl_attack_left.h"
#include "../sprites/girl_attack_right.h"
#include "../sprites/girl_idle.h"
#include "../sprites/zombie1_walk_right.h"
#include "../sprites/zombie1_walk_left.h"
#include "../sprites/zombie1_attack_right.h"
#include "../sprites/zombie1_attack_left.h"
#include "../sprites/zombie1_die_left.h"
#include "../sprites/zombie1_die_right.h"
#include "../sprites/grass_tiles.h"
#include "../sprites/jungle_background.h"
#include "../sprites/hearts.h"
#include "../sprites/skeleton.h"
#include "../ST7789/fonts/FreeMono/FreeMono9pt7b.h"

#define MAX_ENEMIES 4
#define ZOMBIES_WIDTH 54

Game1Scene game1Scene;

const uint16_t* girl_walk_right[] = {
  girl_walk_right_1_bmp,
  girl_walk_right_2_bmp,
  girl_walk_right_3_bmp,
  girl_walk_right_4_bmp,
  girl_walk_right_5_bmp,
  girl_walk_right_6_bmp,
  girl_walk_right_7_bmp,
  girl_walk_right_8_bmp,
  girl_walk_right_9_bmp,
  girl_walk_right_10_bmp
};

const uint16_t* girl_walk_left[] = {
  girl_walk_left_1_bmp,
  girl_walk_left_2_bmp,
  girl_walk_left_3_bmp,
  girl_walk_left_4_bmp,
  girl_walk_left_5_bmp,
  girl_walk_left_6_bmp,
  girl_walk_left_7_bmp,
  girl_walk_left_8_bmp,
  girl_walk_left_9_bmp,
  girl_walk_left_10_bmp
};

const uint16_t* girl_attack_right[] = {
  girl_attack_right_1_bmp,
  girl_attack_right_2_bmp,
  girl_attack_right_3_bmp
};

const uint16_t* girl_attack_left[] = {
  girl_attack_left_1_bmp,
  girl_attack_left_2_bmp,
  girl_attack_left_3_bmp
};

const uint16_t* zombie1_walk_right[] = {
  zombie1_walk_right_1_bmp,
  zombie1_walk_right_2_bmp,
  zombie1_walk_right_3_bmp,
  zombie1_walk_right_4_bmp,
  zombie1_walk_right_5_bmp,
  zombie1_walk_right_6_bmp,
  zombie1_walk_right_7_bmp,
  zombie1_walk_right_8_bmp,
  zombie1_walk_right_9_bmp,
  zombie1_walk_right_10_bmp
};

const uint16_t* zombie1_walk_left[] = {
  zombie1_walk_left_1_bmp,
  zombie1_walk_left_2_bmp,
  zombie1_walk_left_3_bmp,
  zombie1_walk_left_4_bmp,
  zombie1_walk_left_5_bmp,
  zombie1_walk_left_6_bmp,
  zombie1_walk_left_7_bmp,
  zombie1_walk_left_8_bmp,
  zombie1_walk_left_9_bmp,
  zombie1_walk_left_10_bmp
};

const uint16_t* zombie1_attack_right[] = {
  zombie1_attack_right_1_bmp,
  zombie1_attack_right_2_bmp
};

const uint16_t* zombie1_attack_left[] = {
  zombie1_attack_left_1_bmp,
  zombie1_attack_left_2_bmp
};

const uint16_t* zombie1_die_right[] = {
  zombie1_die_right_1_bmp,
  zombie1_die_right_2_bmp
};

const uint16_t* zombie1_die_left[] = {
  zombie1_die_left_1_bmp,
  zombie1_die_left_2_bmp
};

const uint16_t* hearts[] = {
  heart_empty_bmp,
  heart_half_bmp,
  heart_full_bmp
};

unsigned long tickTimer, idleTimer, controllerTimer;

const uint16_t bgColor = 0xc40d;

enum CharacterDirection : uint8_t {
  DIR_LEFT,
  DIR_RIGHT,
  DIR_COUNT
};

enum EnemyType : uint8_t {
  ENEMY_ZOMBIE1,
  ENEMY_ZOMBIE2,
  ENEMY_ZOMBIE3
};


struct Player {
  uint8_t x = 103, lastX = 0;
  uint8_t positionIndex = 0;
  uint8_t direction = DIR_RIGHT;
  uint8_t steps = 2;
  uint8_t speed = 2;
  uint8_t damage = 10;
  uint8_t maxHp = 8; // Always must be an even number
  uint8_t hp = 8;
  uint8_t score = 0;
  bool moving = false;
  bool attacking = false;
};

struct Enemy {
  int16_t x = 0, lastX = 0;
  uint8_t positionIndex = 0;
  uint8_t direction = DIR_RIGHT;
  uint8_t steps = 1;
  uint8_t speed = 10;
  uint8_t type = ENEMY_ZOMBIE1;
  uint8_t health = 10;
  unsigned long tick;
  bool active = false;
  bool moving = false;
  bool attacking = false;
};

Player girl;
Enemy enemies[MAX_ENEMIES];

uint16_t nearestEnemyFromLeft = 0,
nearestEnemyFromRight = 240;

uint8_t livingEnemies = 2;

void setSpeed(uint8_t speed){
  girl.speed = speed * 2;
  girl.steps = speed > 1 ? 3 : 2;
}

void updateHealth() {
  uint8_t heartCount = girl.maxHp >> 1;
  uint8_t hp = girl.hp;

  for(uint8_t i = 0; i < heartCount; i++) {
    uint8_t heartState = 0; // Empty
    int8_t localHp = hp - (i << 1);

    if(localHp >= 2)
      heartState = 2; // Full
    else if(localHp == 1)
      heartState = 1; // Half

    tft.drawRgbBitmap(10 + i * 24, 10, 22, 21, hearts[heartState]);
  }
}

void updateScore(){
  tft.setTextColor(0x0000);
  tft.setFont(&FreeMono9pt7b);
  tft.fillRect(173, 11, 33, 14, 0xef36);
  tft.printWithOffsetAlignment(girl.score, ST7789_ALIGN_CENTER | ST7789_ALIGN_TOP, 72, 7);
}

uint8_t spawnEnemy(uint8_t direction, uint8_t type){
  for(int i = 0; i < MAX_ENEMIES; i++){
    if(!enemies[i].active){
      Enemy newEnemy;
      newEnemy.active = true;
      newEnemy.moving = true;
      newEnemy.type = type;
      newEnemy.direction = direction;
      newEnemy.x = direction == DIR_RIGHT ? -30 : 230;
      newEnemy.lastX = newEnemy.x;
      enemies[i] = newEnemy;
      return i;
    }
  }
  return 0;
}

void renderCharacter(){

  // Handle controller movement
  if(input.now() > controllerTimer){
    // Move the character based on the speed
    controllerTimer = input.now() + (150 / girl.speed);
    
    #ifdef ATTACH_DEBUGGER
      // Just for debug and test
      if(input.pressed(0x31)){
        setSpeed(1);
      }
      else if(input.pressed(0x32)){
        setSpeed(2);
      }
      else if(input.pressed(0x33)){
        setSpeed(3);
      }
      else if(input.pressed(0x34)){
        setSpeed(4);
      }
      else if(input.pressed(0x35)){
        setSpeed(5);
      }
      else if(input.pressed(0x36)){
        setSpeed(6);
      }
    #endif

    if(!girl.moving)
      girl.attacking = input.joystickClicked() || input.enter();

    // Go to left
    if(input.joystickReachedLeft() || input.pressedLeft()) {
      // Last position to clear the character
      girl.lastX = girl.x;
      // New position
      girl.x = girl.x > (nearestEnemyFromLeft + girl.steps) ? (girl.x - girl.steps) : nearestEnemyFromLeft;
      // Set direction to left
      girl.direction = DIR_LEFT;
      // Re-draw the character
      girl.moving = true;
      // Go to idle after 100ms of stopping
      idleTimer = input.now() + 100;
    }

    // Go to right
    else if(input.joystickReachedRight() || input.pressedRight()) {
      girl.lastX = girl.x;
      girl.x = (girl.x + 34) < (nearestEnemyFromRight - girl.steps) ? (girl.x + girl.steps) : (nearestEnemyFromRight - 34);
      girl.direction = DIR_RIGHT;
      girl.moving = true;
      idleTimer = input.now() + 100;
    }

    // Idle
    else{
      if(input.now() > idleTimer){
        // girl.direction = girl.direction == DIR_LEFT ? DIR_IDLE2 : DIR_IDLE1;
        girl.moving = false;
      }
    }

  }

  if(input.now() > tickTimer){
    // Re-draw the character based on its movement speed
    tickTimer = input.now() + (150 / girl.speed);
    
    // Redraw character movement
    if(girl.moving){
      // Clean the last position by filling with the background color
      if(girl.direction == DIR_LEFT){
        tft.fillRect(girl.x + 34, 137, 10, 65, bgColor);
        tft.drawSprite(girl.x, 137, 34, 65, girl_walk_left[girl.positionIndex]);
      }
      else if(girl.direction == DIR_RIGHT){
        tft.fillRect(girl.lastX, 137, 10, 65, bgColor);
        tft.drawSprite(girl.x, 137, 34, 65, girl_walk_right[girl.positionIndex]);
      }
    }

    // Redraw character idle / attacking
    else{
      if(girl.attacking){
        if(girl.positionIndex >= 3){
          girl.positionIndex = 0;
          tickTimer = input.now() + (350 / girl.speed);
        }
        tft.drawSprite(girl.x, 137, 34, 64, girl.direction == DIR_LEFT ? girl_attack_left[girl.positionIndex] : girl_attack_right[girl.positionIndex]);
      }
      else{
        if(girl.direction == DIR_LEFT)
          tft.drawSprite(girl.x, 137, 34, 65, girl_idle_2_bmp);
        else if(girl.direction == DIR_RIGHT)
          tft.drawSprite(girl.x, 137, 34, 65, girl_idle_1_bmp);
      }
    }

    // Draw the character based on its movement direction
    // if(girl.direction == DIR_LEFT)
    //   tft.drawSprite(girl.x, 137, 34, 65, girl_walk_left[girl.positionIndex]);
    // else if(girl.direction == DIR_RIGHT)
    //   tft.drawSprite(girl.x, 137, 34, 65, girl_walk_right[girl.positionIndex]);
    // else if(girl.direction == DIR_IDLE1)
    //   tft.drawSprite(girl.x, 137, 34, 65, girl_idle_1_bmp);
    // else if(girl.direction == DIR_IDLE2)
    //   tft.drawSprite(girl.x, 137, 34, 65, girl_idle_2_bmp);

    girl.positionIndex++;
    
    // Only 10 different position for the character movement
    if(girl.positionIndex >= 10)
      girl.positionIndex = 0;
    
  }
}

void renderEnemy(Enemy* e){

  if(input.now() > e->tick){
  
    e->tick = input.now() + (1750 / e->speed);

    // Draw enemy movement
    if(e->moving){
      if(e->direction == DIR_RIGHT)
        tft.fillRect(e->lastX, 140, 10, 30, bgColor);
      else
        tft.fillRect(e->lastX + ZOMBIES_WIDTH, 140, 10, 30, bgColor);
      tft.drawSprite(e->x, 127, 54, 75, e->direction == DIR_RIGHT ? zombie1_walk_right[e->positionIndex] : zombie1_walk_left[e->positionIndex]);
    }

    // Draw enemy attack
    else if(e->attacking){
      if(e->direction == DIR_RIGHT)
        tft.fillRect(e->lastX, 140, 10, 30, bgColor);
      else
        tft.fillRect(e->lastX + ZOMBIES_WIDTH, 140, 10, 30, bgColor);
      tft.drawSprite(e->x, 127, 54, 75, e->direction == DIR_RIGHT ? zombie1_attack_right[e->positionIndex] : zombie1_attack_left[e->positionIndex]);
    }

    // Draw enemy death
    else if(e->health == 0){
      if(e->positionIndex == 0){
        tft.fillRect(e->lastX, 127, 54, 75, bgColor);
        tft.drawSprite(e->x, 127, 42, 75, e->direction == DIR_RIGHT ? zombie1_die_right[0] : zombie1_die_left[0]);
        e->positionIndex++;
      }
      else if(e->positionIndex == 1){
        tft.fillRect(e->lastX, 127, 54, 75, bgColor);
        tft.drawSprite(e->x, 127, 42, 75, e->direction == DIR_RIGHT ? zombie1_die_right[1] : zombie1_die_left[1]);
        e->positionIndex++;
        e->tick += 1500;
      }
      else if(e->positionIndex == 2){
        if(e->direction == DIR_LEFT){
          tft.fillRect(e->x, 127, 54, 75, bgColor);
          nearestEnemyFromRight = tft.width();
        }
        else{
          tft.fillRect(e->x, 127, 54, 75, bgColor);
          nearestEnemyFromLeft = 0;
        }
        e->active = false;
      }
      return;
    }
  
    bool shouldGo = false;
    if(e->direction == DIR_RIGHT){

      // Update the position of nearest enemy to the girl from right
      if(nearestEnemyFromLeft < e->x + ZOMBIES_WIDTH)
        nearestEnemyFromLeft = e->x + ZOMBIES_WIDTH;

      shouldGo = e->x + ZOMBIES_WIDTH < girl.x;

    }

    else if(e->direction == DIR_LEFT){
      
      // Update the position of nearest enemy to the girl from left
      if(nearestEnemyFromRight > e->x)
        nearestEnemyFromRight = e->x;

      shouldGo = e->x > girl.x + 34;

    }

    // Didn't reach the girl
    if(shouldGo){
      e->lastX = e->x;
      if(e->direction == DIR_RIGHT)
        e->x += e->steps;
      else
        e->x -= e->steps;
      if(!e->moving){
        e->moving = true;
        e->attacking = false;
        e->speed = 10;
      }
    }

    // Reached the girl
    else{
      if(girl.attacking && girl.direction != e->direction && e->health > 0){
        if(e->health <= girl.damage)
          e->health = 0;
        else
          e->health -= girl.damage;
        if(e->health == 0){
          girl.score += 1;
          e->moving = false;
          e->attacking = false;
          e->positionIndex = 0;
          e->speed = 5;
          updateScore();
        }
      }
      else{
        if(!e->attacking){
          e->positionIndex = 0;
          e->moving = false;
          e->attacking = true;
          e->speed = 2;
        }
        else{
          // Damage the girl
        }
      }
    }

    // Update position when moving
    if(e->moving){
      e->positionIndex++;
      if(e->positionIndex > 9)
        e->positionIndex = 0;
    }

    // Update position when attacking
    else if(e->attacking){
      e->positionIndex++;
      if(e->positionIndex > 1)
        e->positionIndex = 0;
    }

  }

}

void renderEnemies(){
  uint8_t livingEnemiesCount = 0;
  for(int i = 0; i < MAX_ENEMIES; i++){
    if(!enemies[i].active)
      continue;
    renderEnemy(&enemies[i]);
    livingEnemiesCount++;
  }
  if(livingEnemiesCount < livingEnemies){
    CharacterDirection randomDir = static_cast<CharacterDirection>(esp_random() % DIR_COUNT);
    spawnEnemy(randomDir, ENEMY_ZOMBIE1);
  }
}

void Game1Scene::enter() {
  // Draw the scene
  tft.fillScreen(bgColor);
  tft.drawSprite(0, 0, 240, 128, jungle_background_bmp);
  tft.drawSprite(0, 202, 60, 38, grass_tile_left_bmp);
  tft.drawSprite(60, 202, 60, 38, grass_tile_middle_bmp);
  tft.drawSprite(120, 202, 60, 38, grass_tile_middle_bmp);
  tft.drawSprite(180, 202, 60, 38, grass_tile_right_bmp);
  updateScore();
  updateHealth();
  spawnEnemy(DIR_RIGHT, ENEMY_ZOMBIE1);
  spawnEnemy(DIR_LEFT, ENEMY_ZOMBIE1);
  spawnEnemy(DIR_LEFT, ENEMY_ZOMBIE1);
  nearestEnemyFromRight = tft.width();
}

void Game1Scene::update() {}

void Game1Scene::render() {
  renderEnemies();
  renderCharacter();
}

void Game1Scene::exit() {}