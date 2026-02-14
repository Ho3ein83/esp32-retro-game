#pragma once

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "gfxfont.h"
#include "commands.h"
#include "macros.h"
#include "colors.h"
#include <stdint.h>

class ST7789 {
private:

    // Pins
    int _dc, _rst, _bl;
    int _sclk, _mosi, _miso;

    // Display
    int _width, _height;

    // SPI
    spi_device_handle_t _spi;
    int _spi_host;
    int _spi_freq;

    // DMA
    int _dma_lines;

    // DMA buffer
    uint16_t *_dma_buf;

    // Low-level helpers
    inline void dc_cmd();
    inline void dc_data();

    void spi_write(const uint8_t *data, int len);
    void write_cmd(uint8_t cmd);
    void write_data(const uint8_t *data, int len);

    void alloc_dma_buffer();
    void set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

    void st7789_reset();
    void st7789_init();

    // Font
    const GFXfont *_font = nullptr;
    uint16_t _textColor = 0xFFFF;
    uint16_t _cursorX = 0, _cursorY = 0;

    // Screen Options
    uint8_t _madctl = MADCTL_BGR;
    bool _colorOrderBGR = true;
    uint8_t _rotation = 0;

    // PWM channel
    ledc_channel_t _bl_channel;

public:

    ST7789(
        int dc,
        int rst,
        int bl,
        int sclk,
        int mosi,
        int miso,
        int width,
        int height,
        int spi_host = SPI2_HOST,
        int spi_freq = 40,
        int dma_lines = 20
    );

    // ---- Initialization ---- //
    void begin();

    // ---- Screen Preferences ---- //
    void setRotation(uint8_t rotation);
    void setInversion(bool enable);
    void setColorOrderRGB();
    void setColorOrderBGR();

    // ---- Font & Printing ---- //
    void setFont(const GFXfont *font);
    void setTextColor(uint16_t color);
    void measureText(const char* text, int16_t& w, int16_t& h);
    void drawChar(int16_t x, int16_t y, char c, uint16_t color);
    void drawChar(int16_t x, int16_t y, char c);
    void drawText(int16_t x, int16_t y, const char *text);
    void drawFastRun(int16_t x, int16_t y, int16_t len, uint16_t colorLE);
    void print(const char* text);
    void print(uint32_t value);
    void print(uint16_t value);
    void print(uint8_t value);
    void print(const char* text, uint8_t alignment);
    void print(uint32_t value, uint8_t alignment);
    void print(uint16_t value, uint8_t alignment);
    void print(uint8_t value, uint8_t alignment);
    void printWithOffsetAlignment(const char* text, uint8_t alignment, int16_t xOffset = 0, int16_t yOffset = 0);
    void printWithOffsetAlignment(uint32_t value, uint8_t alignment, int16_t xOffset = 0, int16_t yOffset = 0);
    void printWithOffsetAlignment(uint16_t value, uint8_t alignment, int16_t xOffset = 0, int16_t yOffset = 0);
    void printWithOffsetAlignment(uint8_t value, uint8_t alignment, int16_t xOffset = 0, int16_t yOffset = 0);
    void setCursor(uint16_t x, uint16_t y);
    
    // ---- Drawing ---- //
    void fillScreen(uint16_t color);
    void drawPixel(int16_t x, int16_t y, uint16_t color);

    // ---- Bitmaps ---- //
    void drawSprite(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *bitmap);
    void drawRgbBitmap(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *bitmap);

    // ---- Shapes & Lines ---- //
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void drawVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
    void drawHLine(int16_t x, int16_t y, int16_t w, uint16_t color);

    // ---- Backlight ---- //
    void setBrightness(uint8_t brightness);
    void fadeIn(uint16_t timeout);
    void fadeOut(uint16_t timeout);
    
    // ---- Utils ---- //
    uint16_t hsvToRgb565(uint16_t hue);
    int width();
    int height();

};
