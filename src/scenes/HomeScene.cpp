#include <stdint.h>
#include <iterator>
#include "HomeScene.h"
#include "../engine/Globals.h"
#include "../engine/Input.h"
#include "../games/Game1Scene.h"
#include "../sprites/icons.h"
#include "../ST7789/fonts/f04b/f04B_30__9pt7b.h"
#include "../ST7789/fonts/f04b/f04B_30__12pt7b.h"
#include "../ST7789/fonts/f04b/f04B_30__18pt7b.h"

HomeScene homeScene;

#define GAMES_COUNT 3

uint8_t selectedGame = 0;
bool shouldUpdateSelection = true;
unsigned long debounce, releaseDebounce;

const uint16_t* icons[1] = {
  icon_school_girl_bmp
};

void updateArrows() {
  tft.setFont(&f04B_30__18pt7b);

  // 1. Determine states
  bool isAtStart = (selectedGame <= 0);
  bool isAtEnd = (selectedGame >= GAMES_COUNT - 1);

  // 2. Define colors
  uint16_t leftColor  = isAtStart ? ST7789_COLOR_CREAM : ST7789_COLOR_PRIMARY;
  uint16_t rightColor = isAtEnd   ? ST7789_COLOR_CREAM : ST7789_COLOR_PRIMARY;
  uint16_t shadowColor = ST7789_COLOR_BLACK;

  // 3. Draw Left Arrow ("<")
  // Shadow
  tft.setTextColor(isAtStart ? ST7789_COLOR_CREAM : shadowColor);
  tft.setCursor(18, 125); 
  tft.print("<");
  // Main
  tft.setTextColor(leftColor);
  tft.setCursor(15, 122);
  tft.print("<");

  // 4. Draw Right Arrow (">")
  // Shadow
  tft.setTextColor(isAtEnd ? ST7789_COLOR_CREAM : shadowColor);
  tft.setCursor(201, 125);
  tft.print(">");
  // Main
  tft.setTextColor(rightColor);
  tft.setCursor(204, 122);
  tft.print(">");
}

// void updateArrows(){
  
//   tft.setFont(&f04B_30__18pt7b);
  
//   if(selectedGame <= 0){
//     tft.setTextColor(ST7789_COLOR_CREAM);
//   }
//   else{
//     tft.setTextColor(ST7789_COLOR_BLACK);
//   }
//   tft.setCursor(10 + 8, 120 + 5);
//   tft.print("<");

//   if(selectedGame >= GAMES_COUNT-1){
//     tft.setTextColor(ST7789_COLOR_CREAM);
//   }
//   else{
//     tft.setTextColor(ST7789_COLOR_BLACK);
//   }
//   tft.setCursor(209 - 8, 120 + 5);
//   tft.print(">");
  
//   if(selectedGame <= 0){
//     tft.setTextColor(ST7789_COLOR_CREAM);
//   }
//   else{
//     tft.setTextColor(ST7789_COLOR_PRIMARY);
//   }
//   tft.setCursor(7 + 8, 117 + 5);
//   tft.print("<");

//   if(selectedGame >= GAMES_COUNT-1){
//     tft.setTextColor(ST7789_COLOR_CREAM);
//   }
//   else{
//     tft.setTextColor(ST7789_COLOR_PRIMARY);
//   }
//   tft.setCursor(212 - 8, 117 + 5);
//   tft.print(">");
  
// }

void HomeScene::enter() {

  tft.setBrightness(0);
  tft.fillScreen(ST7789_COLOR_CREAM);
  
  tft.fillRect(0, 0, tft.width(), 50, ST7789_COLOR_PRIMARY);
  
  tft.setFont(&f04B_30__9pt7b);
  
  tft.setTextColor(ST7789_COLOR_BLACK);
  tft.setCursor(tft.width() / 2 - 95, 15);
  tft.print("Select a game");
  
  tft.setTextColor(ST7789_COLOR_WHITE);
  tft.setCursor(tft.width() / 2 - 98, 12);
  tft.print("Select a game");

  tft.drawHLine(0, 50, tft.width(), ST7789_COLOR_BLACK);

  updateArrows();

  tft.fadeIn(1275);

}

void HomeScene::update() {
  if(input.now() > releaseDebounce){
    releaseDebounce = input.now() + 50;
    if(input.joystickReleased() && input.released())
      debounce = input.now() + 40;
  }
  if(debounce < input.now()){
    debounce = input.now() + 1000;
    if(input.joystickReachedLeft() || input.pressedLeft()){
      if(selectedGame > 0){
        selectedGame--;
        shouldUpdateSelection = true;
      }
    }
    else if(input.joystickReachedRight() || input.pressedRight()){
      if(selectedGame < GAMES_COUNT-1){
        selectedGame++;
        shouldUpdateSelection = true;
      }
    }
    if(input.joystickClicked() || input.enter()){
      if(selectedGame == 0)
        sceneManager.set(&game1Scene);
    }
  }
}

void HomeScene::render() {
  if(shouldUpdateSelection){
    updateArrows();
    if(selectedGame < std::size(icons))
      tft.drawSprite(70, 95, 100, 100, icons[selectedGame]);
    else
      tft.drawSprite(70, 95, 100, 100, icon_frame_bmp);
    shouldUpdateSelection = false;
  }
}

void HomeScene::exit() {}