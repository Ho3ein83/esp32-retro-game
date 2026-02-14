#include <assert.h>
#include "ST7789.h"
#include "commands.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "utils.h"
#include "../engine/IOHelper.h"
#include "./fonts/FreeMono/FreeMono9pt7b.h"

static const char *TAG = "ST7789";

// ===== Constructor =====
ST7789::ST7789(
    int dc,
    int rst,
    int bl,
    int sclk,
    int mosi,
    int miso,
    int width,
    int height,
    int spi_host,
    int spi_freq,
    int dma_lines
)
: _dc(dc), _rst(rst), _bl(bl),
  _sclk(sclk), _mosi(mosi), _miso(miso),
  _width(width), _height(height),
  _spi(nullptr),
  _spi_host(spi_host),
  _spi_freq(spi_freq),
  _dma_lines(dma_lines),
  _dma_buf(nullptr)
{
}

// ===== Low-level helpers =====
inline void ST7789::dc_cmd()  { gpio_set_level((gpio_num_t)_dc, 0); }
inline void ST7789::dc_data() { gpio_set_level((gpio_num_t)_dc, 1); }

void ST7789::spi_write(const uint8_t *data, int len)
{
    spi_transaction_t t = {};
    t.length = len * 8;
    t.tx_buffer = data;
    ESP_ERROR_CHECK(spi_device_transmit(_spi, &t));
}

void ST7789::write_cmd(uint8_t cmd)
{
    dc_cmd();
    spi_write(&cmd, 1);
}

void ST7789::write_data(const uint8_t *data, int len)
{
    dc_data();
    spi_write(data, len);
}

// ===== DMA =====
void ST7789::alloc_dma_buffer()
{
    _dma_buf = (uint16_t *)heap_caps_malloc(
        _width * _dma_lines * sizeof(uint16_t),
        MALLOC_CAP_DMA
    );
    assert(_dma_buf && "DMA buffer alloc failed");
}

// ===== Init =====
void ST7789::begin()
{
    // GPIO
    gpio_set_direction((gpio_num_t)_dc, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)_rst, GPIO_MODE_OUTPUT);

    alloc_dma_buffer();

    // SPI bus
    spi_bus_config_t buscfg = {};
    buscfg.sclk_io_num = _sclk;
    buscfg.mosi_io_num = _mosi;
    buscfg.miso_io_num = _miso;
    buscfg.max_transfer_sz = _width * _dma_lines * 2 + 8;

    ESP_ERROR_CHECK(spi_bus_initialize((spi_host_device_t)_spi_host, &buscfg, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t devcfg = {};
    devcfg.clock_speed_hz = _spi_freq * 1000 * 1000;
    devcfg.mode = 3;
    devcfg.spics_io_num = -1; // No CS pin on the display
    devcfg.queue_size = 1;
    devcfg.flags = SPI_DEVICE_HALFDUPLEX;

    ESP_ERROR_CHECK(spi_bus_add_device((spi_host_device_t)_spi_host, &devcfg, &_spi));

    st7789_reset();
    st7789_init();

    io.pwmInit(_bl, &_bl_channel);

    setColorOrderRGB();
    setBrightness(255);
    setFont(&FreeMono9pt7b);

    ESP_LOGI(TAG, "ST7789 ready");
}

// ===== ST7789 control =====
void ST7789::st7789_reset()
{
    gpio_set_level((gpio_num_t)_rst, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level((gpio_num_t)_rst, 1);
    vTaskDelay(pdMS_TO_TICKS(120));
}

void ST7789::st7789_init()
{
    _rotation = 0;
    _colorOrderBGR = true;
    _madctl = MADCTL_BGR;

    write_cmd(ST7789_SWRESET);
    vTaskDelay(pdMS_TO_TICKS(150));

    write_cmd(ST7789_SLPOUT);
    vTaskDelay(pdMS_TO_TICKS(120));

    uint8_t colmod = 0x55; // RGB565
    write_cmd(ST7789_COLMOD);
    write_data(&colmod, 1);

    // Apply MADCTL state ONCE
    write_cmd(ST7789_MADCTL);
    write_data(&_madctl, 1);

    write_cmd(ST7789_INVON);
    write_cmd(ST7789_DISPON);
    vTaskDelay(pdMS_TO_TICKS(100));

    // Backlight setup (unchanged)
    // ledc_timer_config_t timer = {};
    // timer.speed_mode = LEDC_LOW_SPEED_MODE;
    // timer.timer_num = LEDC_TIMER_0;
    // timer.duty_resolution = LEDC_TIMER_8_BIT;
    // timer.freq_hz = 5000;
    // timer.clk_cfg = LEDC_AUTO_CLK;
    // ledc_timer_config(&timer);

    // ledc_channel_config_t channel = {};
    // channel.gpio_num = _bl;
    // channel.speed_mode = LEDC_LOW_SPEED_MODE;
    // channel.channel = LEDC_CHANNEL_0;
    // channel.timer_sel = LEDC_TIMER_0;
    // channel.duty = 0;
    // channel.hpoint = 0;
    // ledc_channel_config(&channel);
}

// ===== Font & Printing =====

void ST7789::setFont(const GFXfont *font)
{
    _font = font;
}

void ST7789::setTextColor(uint16_t color)
{
    _textColor = color;
}

void ST7789::measureText(const char* text, int16_t& w, int16_t& h) {
    if (!_font || !text) {
        w = h = 0;
        return;
    }

    int16_t x = 0;
    int16_t minY = 0;
    int16_t maxY = 0;

    while (*text) {
        char c = *text++;
        if (c < _font->first || c > _font->last) continue;

        const GFXglyph* g = &_font->glyph[c - _font->first];

        int16_t y1 = g->yOffset;
        int16_t y2 = g->yOffset + g->height;

        minY = MIN(minY, y1);
        maxY = MAX(maxY, y2);

        x += g->xAdvance;
    }

    w = x;
    h = maxY - minY;
}

void ST7789::drawChar(int16_t x, int16_t y, char c, uint16_t color) {
    if (!_font) return;
    if (c < _font->first || c > _font->last) return;

    const GFXglyph* glyph = &_font->glyph[c - _font->first];
    const uint8_t* bitmap = _font->bitmap + glyph->bitmapOffset;

    int16_t w = glyph->width;
    int16_t h = glyph->height;
    int16_t xo = glyph->xOffset;
    int16_t yo = glyph->yOffset;

    uint8_t bitMask = 0;
    uint8_t bits = 0;
    uint16_t colorLE = (color >> 8) | (color << 8); // Pre-swap endianness

    for (int16_t yy = 0; yy < h; yy++) {
        int16_t runLength = 0;
        int16_t startX = -1;

        for (int16_t xx = 0; xx < w; xx++) {
            if (!(bitMask >>= 1)) {
                bits = *bitmap++;
                bitMask = 0x80;
            }

            if (bits & bitMask) {
                if (runLength == 0) {
                    startX = xx;
                }
                runLength++;
            } else {
                if (runLength > 0) {
                    // Flush the horizontal run
                    drawFastRun(x + xo + startX, y + yo + yy, runLength, colorLE);
                    runLength = 0;
                }
            }
        }
        // Flush any remaining run at the end of the row
        if (runLength > 0) {
            drawFastRun(x + xo + startX, y + yo + yy, runLength, colorLE);
        }
    }
}

// Add this private helper to your class
void ST7789::drawFastRun(int16_t x, int16_t y, int16_t len, uint16_t colorLE) {
    if (x < 0 || x >= _width || y < 0 || y >= _height) return;
    if (x + len > _width) len = _width - x;

    set_window(x, y, x + len - 1, y);

    for (int i = 0; i < len; i++) {
        _dma_buf[i] = colorLE;
    }

    spi_transaction_t t = {};
    t.tx_buffer = _dma_buf;
    t.length = len * 16;

    dc_data();
    ESP_ERROR_CHECK(spi_device_polling_transmit(_spi, &t));
}

// void ST7789::drawChar(int16_t x, int16_t y, char c, uint16_t color) {
//     if (!_font) return;
//     if (c < _font->first || c > _font->last) return;

//     const GFXglyph* glyph = &_font->glyph[c - _font->first];
//     const uint8_t* bitmap = _font->bitmap + glyph->bitmapOffset;

//     int16_t w = glyph->width;
//     int16_t h = glyph->height;
//     int16_t xo = glyph->xOffset - 1;
//     int16_t yo = glyph->yOffset + glyph->height;  // baseline offset

//     uint8_t bitMask = 0;
//     uint8_t bits = 0;

//     for (int16_t yy = 0; yy < h; yy++) {
//         for (int16_t xx = 0; xx < w; xx++) {

//             if (!(bitMask >>= 1)) {
//                 bits = *bitmap++;
//                 bitMask = 0x80;
//             }

//             if (bits & bitMask) {
//                 drawPixel(x + xo + xx, y + yo + yy, color);
//             }
//         }
//     }
// }

void ST7789::drawChar(int16_t x, int16_t y, char c) {
    drawChar(x, y, c, _textColor);
}

void ST7789::print(const char* text) {
    while (*text) {
        char c = *text++;

        if (c == '\n') {
            _cursorX = 0;
            _cursorY += _font->yAdvance;  // move to next line
            continue;
        }

        if (c < _font->first || c > _font->last) continue;

        const GFXglyph* glyph = &_font->glyph[c - _font->first];

        drawChar(_cursorX, _cursorY, c, _textColor);

        _cursorX += glyph->xAdvance;  // horizontal advance
    }
}

void ST7789::print(uint32_t value){
    char buf[12];
    sprintf(buf, "%lu", value); // %lu = unsigned long
    print(buf);
}

void ST7789::print(uint16_t value){
    char buf[6];
    sprintf(buf, "%u", value); // %u = unsigned int
    print(buf);
}

void ST7789::print(uint8_t value){
    char buf[4];
    sprintf(buf, "%u", value);
    print(buf);
}

void ST7789::printWithOffsetAlignment(const char* text, uint8_t alignment, int16_t xOffset, int16_t yOffset) {
    if (!_font || !text) return;

    int16_t textW, textH;
    measureText(text, textW, textH);

    // bool hasVerticalAlign =
    // alignment & (ST7789_ALIGN_TOP |
    //              ST7789_ALIGN_MIDDLE |
    //              ST7789_ALIGN_DOWN);

    int16_t x = _cursorX;
    int16_t y = _cursorY;

    // ---------- Horizontal alignment ----------
    if (alignment & ST7789_ALIGN_CENTER) {
        x = (_width - textW) / 2;
    } 
    else if (alignment & ST7789_ALIGN_LEFT) {
        x = ST7789_MIN_PADDING;
    } 
    else if (alignment & ST7789_ALIGN_RIGHT) {
        x = _width - textW - ST7789_MIN_PADDING;
    }
    // LEFT → keep cursorX

    // ---------- Vertical alignment ----------
    if (alignment & ST7789_ALIGN_TOP) {
        // cursorY is top → convert to baseline
        y = textH + ST7789_MIN_PADDING;
    }
    else if (alignment & ST7789_ALIGN_MIDDLE) {
        y = (_height - textH) / 2 + textH;
    }
    else if (alignment & ST7789_ALIGN_BOTTOM) {
        y = _height - ST7789_MIN_PADDING;
    }
    // default → keep cursorY (baseline)

    // ---------- Render ----------
    int16_t startX = x;
    _cursorX = x + xOffset;
    _cursorY = y + yOffset;

    if(_font)
        y += _font->yAdvance;

    while (*text) {
        if (*text == '\n') {
            _cursorX = startX;
            _cursorY += _font->yAdvance;
        } else {
            drawChar(_cursorX, _cursorY, *text);
            const GFXglyph* g = &_font->glyph[*text - _font->first];
            _cursorX += g->xAdvance;
        }
        text++;
    }
}

void ST7789::printWithOffsetAlignment(uint32_t value, uint8_t alignment, int16_t xOffset, int16_t yOffset) {
    char buf[12];
    sprintf(buf, "%lu", value);
    printWithOffsetAlignment(buf, alignment, xOffset, yOffset);
}

void ST7789::printWithOffsetAlignment(uint16_t value, uint8_t alignment, int16_t xOffset, int16_t yOffset) {
    char buf[6];
    sprintf(buf, "%u", value);
    printWithOffsetAlignment(buf, alignment, xOffset, yOffset);
}

void ST7789::printWithOffsetAlignment(uint8_t value, uint8_t alignment, int16_t xOffset, int16_t yOffset) {
    char buf[4];
    sprintf(buf, "%u", value);
    printWithOffsetAlignment(buf, alignment, xOffset, yOffset);
}

void ST7789::print(const char* text, uint8_t alignment) {
    printWithOffsetAlignment(text, alignment, 0, 0);
}

void ST7789::print(uint32_t value, uint8_t alignment) {
    char buf[12];
    sprintf(buf, "%lu", value);
    printWithOffsetAlignment(buf, alignment, 0, 0);
}

void ST7789::print(uint16_t value, uint8_t alignment) {
    char buf[6];
    sprintf(buf, "%u", value);
    printWithOffsetAlignment(buf, alignment, 0, 0);
}

void ST7789::print(uint8_t value, uint8_t alignment) {
    char buf[4];
    sprintf(buf, "%u", value);
    printWithOffsetAlignment(buf, alignment, 0, 0);
}

void ST7789::setCursor(uint16_t x, uint16_t y){
    _cursorX = x;
    _cursorY = y;
    if(_font)
        _cursorY += _font->yAdvance;
}

// ===== Drawing =====

// void ST7789::set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
// {
//     uint8_t data[4];

//     write_cmd(ST7789_CASET);
//     data[0] = x0 >> 8;
//     data[1] = x0 & 0xFF;
//     data[2] = x1 >> 8;
//     data[3] = x1 & 0xFF;
//     write_data(data, 4);

//     write_cmd(ST7789_RASET);
//     data[0] = y0 >> 8;
//     data[1] = y0 & 0xFF;
//     data[2] = y1 >> 8;
//     data[3] = y1 & 0xFF;
//     write_data(data, 4);

//     write_cmd(ST7789_RAMWR);
// }

void ST7789::set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint8_t data[4];

    write_cmd(ST7789_CASET);
    data[0] = x0 >> 8;
    data[1] = x0 & 0xFF;
    data[2] = x1 >> 8;
    data[3] = x1 & 0xFF;
    write_data(data, 4);

    write_cmd(ST7789_RASET);
    data[0] = y0 >> 8;
    data[1] = y0 & 0xFF;
    data[2] = y1 >> 8;
    data[3] = y1 & 0xFF;
    write_data(data, 4);

    write_cmd(ST7789_RAMWR);
}

void ST7789::drawPixel(int16_t x, int16_t y, uint16_t color)
{
    // Clip pixels outside the screen
    if (x < 0 || x >= _width || y < 0 || y >= _height) return;

    set_window(x, y, x, y);  // set window to 1 pixel

    uint16_t c = (color >> 8) | (color << 8); // endian swap
    dc_data();

    spi_transaction_t t = {};
    t.tx_buffer = &c;
    t.length    = 16;  // bits
    ESP_ERROR_CHECK(spi_device_polling_transmit(_spi, &t));
}


void ST7789::fillScreen(uint16_t color)
{
    set_window(0, 0, _width - 1, _height - 1);

    uint16_t c = (color >> 8) | (color << 8);

    for (int i = 0; i < _width * _dma_lines; i++) {
        _dma_buf[i] = c;
    }

    spi_transaction_t t = {};
    t.tx_buffer = _dma_buf;

    dc_data();

    int remaining = _height;
    while (remaining > 0) {
        int lines = (remaining > _dma_lines) ? _dma_lines : remaining;
        t.length = lines * _width * 16;
        ESP_ERROR_CHECK(spi_device_polling_transmit(_spi, &t));
        remaining -= lines;
    }
}

// ===== Bitmaps =====

void ST7789::drawSprite(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *bitmap)
{
    if (!bitmap || w <= 0 || h <= 0) return;

    int16_t original_w = w; // Store the true width of the bitmap data
    int16_t x0 = 0;
    int16_t y0 = 0;

    // ---- Clip X ----
    if (x < 0) { x0 = -x; w += x; x = 0; }
    if (x >= _width) return;
    if (x + w > _width) w = _width - x;

    // ---- Clip Y ----
    if (y < 0) { y0 = -y; h += y; y = 0; }
    if (y >= _height) return;
    if (y + h > _height) h = _height - y;

    if (w <= 0 || h <= 0) return;

    int remaining_h = h;
    int current_y = y;

    // Important: The source starts at the clipped offset
    // but moves based on the original width (stride)
    const uint16_t *src_line = bitmap + (y0 * original_w) + x0;

    while (remaining_h > 0) {
        int lines_to_draw = (remaining_h > _dma_lines) ? _dma_lines : remaining_h;
        
        // We fill the DMA buffer row by row to handle the stride jump
        for (int row = 0; row < lines_to_draw; row++) {
            for (int col = 0; col < w; col++) {
                uint16_t c = src_line[row * original_w + col];
                _dma_buf[row * w + col] = (c >> 8) | (c << 8);
            }
        }

        set_window(x, current_y, x + w - 1, current_y + lines_to_draw - 1);

        spi_transaction_t t = {};
        t.tx_buffer = _dma_buf;
        t.length = w * lines_to_draw * 16;

        dc_data();
        ESP_ERROR_CHECK(spi_device_polling_transmit(_spi, &t));

        // Advance the source line pointer by the number of lines processed * original width
        src_line += (lines_to_draw * original_w);
        current_y += lines_to_draw;
        remaining_h -= lines_to_draw;
    }
}

// void ST7789::drawSprite(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *bitmap)
// {
//     if (!bitmap || w <= 0 || h <= 0) return;

//     // ---- Clip X ----
//     int16_t x0 = 0;
//     if (x < 0) {
//         x0 = -x;
//         w += x;
//         x = 0;
//     }
//     if (x >= _width) return;
//     if (x + w > _width) {
//         w = _width - x;
//     }

//     // ---- Clip Y ----
//     int16_t y0 = 0;
//     if (y < 0) {
//         y0 = -y;
//         h += y;
//         y = 0;
//     }
//     if (y >= _height) return;
//     if (y + h > _height) {
//         h = _height - y;
//     }

//     if (w <= 0 || h <= 0) return;

//     int remaining = h;
//     int yy = y;
//     const uint16_t *src = bitmap + y0 * w + x0;

//     while (remaining > 0) {
//         int lines = (remaining > _dma_lines) ? _dma_lines : remaining;
//         int pixels = w * lines;

//         // Copy bitmap → DMA buffer (with endian swap)
//         for (int i = 0; i < pixels; i++) {
//             uint16_t c = src[i];
//             _dma_buf[i] = (c >> 8) | (c << 8);
//         }

//         set_window(x, yy, x + w - 1, yy + lines - 1);

//         spi_transaction_t t = {};
//         t.tx_buffer = _dma_buf;
//         t.length = pixels * 16;

//         dc_data();
//         ESP_ERROR_CHECK(spi_device_polling_transmit(_spi, &t));

//         src += w * lines;
//         yy += lines;
//         remaining -= lines;
//     }
// }

// ===== Shapes & Lines =====

void ST7789::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    if (w <= 0 || h <= 0) return;

    // ---- Clip X ----
    if (x < 0) {
        w += x;
        x = 0;
    }
    if (x >= _width) return;
    if (x + w > _width) {
        w = _width - x;
    }

    // ---- Clip Y ----
    if (y < 0) {
        h += y;
        y = 0;
    }
    if (y >= _height) return;
    if (y + h > _height) {
        h = _height - y;
    }

    if (w <= 0 || h <= 0) return;

    uint16_t c = (color >> 8) | (color << 8);

    int remaining = h;
    int yy = y;

    while (remaining > 0) {
        int lines = (remaining > _dma_lines) ? _dma_lines : remaining;

        // Fill FULL DMA buffer: width × lines
        int pixels = w * lines;
        for (int i = 0; i < pixels; i++) {
            _dma_buf[i] = c;
        }

        set_window(x, yy, x + w - 1, yy + lines - 1);

        spi_transaction_t t = {};
        t.tx_buffer = _dma_buf;
        t.length = pixels * 16;

        dc_data();
        ESP_ERROR_CHECK(spi_device_polling_transmit(_spi, &t));

        yy += lines;
        remaining -= lines;
    }
}

void ST7789::drawRgbBitmap(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *bitmap)
{
    if (!bitmap || w <= 0 || h <= 0) return;

    for (int16_t row = 0; row < h; row++) {
        int16_t yy = y + row;
        if (yy < 0 || yy >= _height) continue;

        int count = 0;
        int start = -1;

        for (int16_t col = 0; col < w; col++) {
            int16_t xx = x + col;
            if (xx < 0 || xx >= _width) continue;

            uint16_t c = bitmap[row * w + col];
            if (c == 0x0001) {
                if (count > 0) {
                    // Flush previous run
                    set_window(x + start, yy, x + start + count - 1, yy);

                    spi_transaction_t t = {};
                    t.tx_buffer = _dma_buf;
                    t.length = count * 16;

                    dc_data();
                    ESP_ERROR_CHECK(spi_device_polling_transmit(_spi, &t));

                    count = 0;
                }
                continue;
            }

            if (count == 0) {
                start = col;
            }

            _dma_buf[count++] = (c >> 8) | (c << 8);
        }

        // Flush tail run
        if (count > 0) {
            set_window(x + start, yy, x + start + count - 1, yy);

            spi_transaction_t t = {};
            t.tx_buffer = _dma_buf;
            t.length = count * 16;

            dc_data();
            ESP_ERROR_CHECK(spi_device_polling_transmit(_spi, &t));
        }
    }
}

void ST7789::drawVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
{
    if (x < 0 || x >= _width || h <= 0) return;

    // Clip Y
    if (y < 0) {
        h += y;
        y = 0;
    }
    if (y + h > _height) {
        h = _height - y;
    }
    if (h <= 0) return;

    uint16_t c = (color >> 8) | (color << 8);

    int remaining = h;
    int yy = y;

    while (remaining > 0) {
        int lines = (remaining > _dma_lines) ? _dma_lines : remaining;

        set_window(x, yy, x, yy + lines - 1);

        for (int i = 0; i < lines; i++) {
            _dma_buf[i] = c;
        }

        spi_transaction_t t = {};
        t.tx_buffer = _dma_buf;
        t.length = lines * 16;

        dc_data();
        ESP_ERROR_CHECK(spi_device_polling_transmit(_spi, &t));

        yy += lines;
        remaining -= lines;
    }
}

void ST7789::drawHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
    if (y < 0 || y >= _height || w <= 0) return;

    // Clip X
    if (x < 0) {
        w += x;
        x = 0;
    }
    if (x + w > _width) {
        w = _width - x;
    }
    if (w <= 0) return;

    set_window(x, y, x + w - 1, y);

    uint16_t c = (color >> 8) | (color << 8);

    // Fill DMA buffer
    for (int i = 0; i < w; i++) {
        _dma_buf[i] = c;
    }

    spi_transaction_t t = {};
    t.tx_buffer = _dma_buf;
    t.length = w * 16;

    dc_data();
    ESP_ERROR_CHECK(spi_device_polling_transmit(_spi, &t));
}

// ===== Backlight =====

void ST7789::setBrightness(uint8_t brightness){
    io.pwmWrite(_bl_channel, brightness);
    // ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, brightness);
    // ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

void ST7789::fadeIn(uint16_t timeout)
{
    uint16_t wait = timeout / 255;
    
    for(int brightness = 0; brightness <= 255; brightness++){
        setBrightness(brightness);
        vTaskDelay(pdMS_TO_TICKS(wait));
    }
}

void ST7789::fadeOut(uint16_t timeout)
{
    uint16_t wait = timeout / 255;

    for (int brightness = 255; brightness > 0; brightness--) {
        setBrightness(brightness);
        vTaskDelay(pdMS_TO_TICKS(wait));
    }

    setBrightness(0);
}

// ===== Screen Preferences =====

void ST7789::setRotation(uint8_t rotation)
{
    _rotation = rotation & 3;

    _madctl &= ~(MADCTL_MX | MADCTL_MY | MADCTL_MV);

    switch (_rotation) {
        case 0:
            // Portrait
            break;

        case 1:
            _madctl |= MADCTL_MV | MADCTL_MX;
            break;

        case 2:
            _madctl |= MADCTL_MX | MADCTL_MY;
            break;

        case 3:
            _madctl |= MADCTL_MV | MADCTL_MY;
            break;
    }

    // Preserve color order
    if (_colorOrderBGR)
        _madctl |= MADCTL_BGR;
    else
        _madctl &= ~MADCTL_BGR;

    write_cmd(ST7789_MADCTL);
    write_data(&_madctl, 1);
}

void ST7789::setInversion(bool enable) {
    write_cmd(enable ? ST7789_INVON : ST7789_INVOFF);
}

void ST7789::setColorOrderRGB()
{
    _colorOrderBGR = false;
    _madctl &= ~MADCTL_BGR;

    write_cmd(ST7789_MADCTL);
    write_data(&_madctl, 1);
}

void ST7789::setColorOrderBGR()
{
    _colorOrderBGR = true;
    _madctl |= MADCTL_BGR;

    write_cmd(ST7789_MADCTL);
    write_data(&_madctl, 1);
}

// ===== Utils =====

uint16_t ST7789::hsvToRgb565(uint16_t hue) {
    
    hue %= 360;
    uint8_t rgb[3];
    uint8_t region = hue / 60;
    
    // Use fixed-point math to avoid floating point overhead
    uint8_t remainder = (hue % 60) * 255 / 60; 

    uint8_t q = 255 - remainder;
    uint8_t t = remainder;

    switch (region) {
        case 0: rgb[0] = 255; rgb[1] = t;   rgb[2] = 0;   break;
        case 1: rgb[0] = q;   rgb[1] = 255; rgb[2] = 0;   break;
        case 2: rgb[0] = 0;   rgb[1] = 255; rgb[2] = t;   break;
        case 3: rgb[0] = 0;   rgb[1] = q;   rgb[2] = 255; break;
        case 4: rgb[0] = t;   rgb[1] = 0;   rgb[2] = 255; break;
        default:rgb[0] = 255; rgb[1] = 0;   rgb[2] = q;   break;
    }

    // Fast bit-packing for RGB565
    return ((rgb[0] & 0xF8) << 8) | ((rgb[1] & 0xFC) << 3) | (rgb[2] >> 3);

}

int ST7789::width(){
    return _width;
}

int ST7789::height(){
    return _height;
}