// ===== Attach the debugger ===== //
#define ATTACH_DEBUGGER

#include <string>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "utils.h"

// ===== Display driver ===== //
#include "./ST7789/ST7789.h"

#include "./ST7789/fonts/f04b/f04B_30__18pt7b.h"

// ===== Engine ===== //
#include "engine/Globals.h"
#include "engine/IOHelper.h"
#include "scenes/SplashScene.h"
#include "scenes/HomeScene.h"
#include "engine/Input.h"

// ===== Pinout ===== //
#define JOYSTICK_X  5
#define JOYSTICK_Y  4
#define JOYSTICK_B  6
#define STATUS_LED  35
#define TFT_DC      10
#define TFT_RST     9
#define TFT_SCLK    12
#define TFT_MISO    13
#define TFT_MOSI    11
#define TFT_BL      21
#define TFT_WIDTH   240 // Other resolutions are not supported, just for calcultions 
#define TFT_HEIGHT  240 // Other resolutions are not supported, just for calcultions

// ===== Configuration ===== //
#define SHOW_SPLASH     false

#ifdef ATTACH_DEBUGGER
#include "engine/RetroDebugger.h"
#endif

SceneManager sceneManager;

IOHelper io;

ST7789 tft(
        TFT_DC,
        TFT_RST,
        TFT_BL,
        TFT_SCLK,
        TFT_MOSI,
        TFT_MISO,
        TFT_WIDTH, // Hard coded internally, just for rendering and calculations
        TFT_HEIGHT, // Hard coded internally, just for rendering and calculations
        SPI2_HOST, // HSPI (default)
        80, // SPI frequency in MHz (default is 40)
        40 // DMA lines for faster data transfer (default is 40)
    );

RetroDebugger debugger;

// ===== Main ===== //
extern "C" void app_main()
{

    io.pinMode(STATUS_LED, OUTPUT);

    io.digitalWrite(STATUS_LED, 1);

    tft.begin();

    input.begin(JOYSTICK_X, JOYSTICK_Y, JOYSTICK_B);

    #ifdef ATTACH_DEBUGGER
    debugger.setup();
    #endif

    #if SHOW_SPLASH
    sceneManager.set(&splashScene);
    #else
    sceneManager.set(&homeScene);
    #endif

    while(true){
        input.update();
        sceneManager.update();
        sceneManager.render();
        #ifdef ATTACH_DEBUGGER
        debugger.debug();
        #endif

        vTaskDelay(1);
    }

}
