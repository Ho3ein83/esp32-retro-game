#pragma once

#include <stdint.h>
#include "driver/gpio.h"
#include "IOHelper.h"
#include "esp_timer.h"
#include "driver/usb_serial_jtag.h"

#define JOYCALIB_X_RIGHT 0
#define JOYCALIB_X_IDLE 1900
#define JOYCALIB_X_LEFT 4095
#define JOYCALIB_X_THRESHOLD 180
#define JOYCALIB_Y_UP 150
#define JOYCALIB_Y_IDLE 535
#define JOYCALIB_Y_DOWN 910
#define JOYCALIB_Y_THRESHOLD 90

class Input {
public:
  uint8_t readData = 0x00;
  gpio_num_t _joystick_x, _joystick_y, _joystick_b;

  void begin(uint8_t joystickXPin, uint8_t joystickYPin, uint8_t joystickBPin){
    _joystick_x = (gpio_num_t) joystickXPin;
    _joystick_y = (gpio_num_t) joystickYPin;
    _joystick_b = (gpio_num_t) joystickBPin;
    io.pinMode(_joystick_b, INPUT_PULLUP);

    usb_serial_jtag_driver_config_t config = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
    esp_err_t err = usb_serial_jtag_driver_install(&config);
    ESP_ERROR_CHECK(err);
  
  }
  
  void update() {
    uint8_t readBuffer[1];
    int read = usb_serial_jtag_read_bytes(readBuffer, sizeof(readBuffer), 10);
    if(read){
      readData = readBuffer[0];
      if(readBuffer[0] == 0xAA){
          printf("#Synced\n");
          #ifdef ATTACH_DEBUGGER
          printf("#memory_monitor\n");
          #endif
      }
    }
    // if(Serial.available()){
    //   readData = Serial.read();
    //   if(readData == 0xAA){
    //     readData = 0x00;
    //     #ifdef ATTACH_DEBUGGER
    //       Serial.println("#Synced");
    //     #endif
    //     return;
    //   }
    //   else if(readData == 0x00){
    //     // Serial.println("Released");
    //     return;
    //   }
    //   #ifdef ATTACH_DEBUGGER
    //     Serial.print(readData, DEC);
    //     Serial.print(" - 0x");
    //     Serial.println(readData, HEX);
    //   #endif
    // }
    // printf("X: %d\n", readJoystickX());
    // printf("Y: %d\n", readJoystickY());
    // printf("B: %d\n", joystickClicked() ? 1 : 0);
  }

  bool joystickClicked(){
    return _joystick_b ? !io.digitalRead(_joystick_b) : false;
  }

  uint16_t readJoystickX(){
    return io.analogRead(_joystick_x);
  }

  uint16_t readJoystickY(){
    return io.analogRead(_joystick_y);
  }

  bool joystickReachedLeft(){
    uint16_t x = readJoystickX();
    return x >= (JOYCALIB_X_LEFT - JOYCALIB_X_THRESHOLD);
  }

  bool joystickReachedRight(){
    uint16_t x = readJoystickX();
    return x <= (JOYCALIB_X_RIGHT + JOYCALIB_X_THRESHOLD);
  }

  bool joystickReachedUp(){
    uint16_t y = readJoystickY();
    return y <= (JOYCALIB_Y_UP + JOYCALIB_Y_THRESHOLD);
  }

  bool joystickReachedDown(){
    uint16_t y = readJoystickY();
    return y >= (JOYCALIB_Y_DOWN - JOYCALIB_Y_THRESHOLD);
  }

  bool joystickReleased(){
    uint16_t x = readJoystickX();
    uint16_t y = readJoystickY();
    return x >= (JOYCALIB_X_IDLE - JOYCALIB_X_THRESHOLD) && x <= (JOYCALIB_X_IDLE + JOYCALIB_X_THRESHOLD)
        && y >= (JOYCALIB_Y_IDLE - JOYCALIB_Y_THRESHOLD) && y <= (JOYCALIB_Y_IDLE + JOYCALIB_Y_THRESHOLD);
  }

  bool pressed(uint8_t c) {
    return readData == c;
  }

  bool pressedLeft() {
    return readData == 0x10;
  }

  bool pressedRight() {
    return readData == 0x11;
  }

  bool enter() {
    return readData == 0x14;
  }

  bool released(){
    return readData == 0x00;
  }

  unsigned long now(){
    return esp_timer_get_time() / 1000;
  }

  uint8_t read(){
    return readData;
  }

};

extern Input input;
