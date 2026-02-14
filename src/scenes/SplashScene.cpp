#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "SplashScene.h"
#include "../scenes/HomeScene.h"
#include "../engine/Globals.h"
#include "../engine/Input.h"
#include "../games/Game1Scene.h"
#include "../ST7789/fonts/f04b/f04B_30__9pt7b.h"
#include "../ST7789/fonts/f04b/f04B_30__12pt7b.h"
#include "../ST7789/fonts/f04b/f04B_30__18pt7b.h"
#include "../sprites/red_guy_menu.h"

SplashScene splashScene;

bool once = false;

void SplashScene::enter() {
  
  tft.setBrightness(0);
  tft.fillScreen(ST7789_COLOR_WHITE);
  tft.drawSprite(tft.width() / 2 - 27, tft.height() - 140, 58, 140, red_guy_menu_bmp);

  tft.setFont(&f04B_30__18pt7b);
  tft.setTextColor(ST7789_COLOR_BLACK);
  tft.setCursor(tft.width() / 2 - 92, 10);
  tft.print("HO3EIN");
  tft.setTextColor(ST7789_COLOR_PRIMARY);
  tft.setCursor(tft.width() / 2 - 88, 14);
  tft.print("HO3EIN");
  
  tft.setFont(&f04B_30__12pt7b);
  tft.setTextColor(ST7789_COLOR_BLACK);
  tft.setCursor(18, 55);
  tft.print("Retro Game");
  tft.setTextColor(ST7789_COLOR_RED);
  tft.setCursor(20, 57);
  tft.print("Retro Game");
  
  tft.fadeIn(3000);
  vTaskDelay(pdMS_TO_TICKS(1500));
  tft.fadeOut(3000);
  vTaskDelay(pdMS_TO_TICKS(600));

}

void SplashScene::update() {
  if(!once){
    sceneManager.set(&homeScene);
    once = true;
  }
}

void SplashScene::render() {
}

void SplashScene::exit() {}