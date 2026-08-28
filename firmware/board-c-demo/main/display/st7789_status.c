#include "st7789_status.h"

#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_commands.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "board_c_pins.h"
#include "font.h"
#include "protocol/asr_event.h"
#include "status_text.h"

#define LCD_HRES 240
#define LCD_VRES 240
#define COLOR_BG 0x10A4
#define COLOR_TITLE 0xE73C
#define COLOR_OK 0x07E0
#define COLOR_WAIT 0xFE60
#define COLOR_BAD 0xF800
#define COLOR_MUTED 0xAD55

static const char *TAG = "board_c_lcd";
static esp_lcd_panel_handle_t s_panel;
static esp_lcd_panel_io_handle_t s_io;
static uint16_t s_line[LCD_HRES];
static uint16_t s_tile[VENTURED_FONT_ROWS * VENTURED_FONT_COLS];
static bool s_ready;
static SemaphoreHandle_t s_lock;
static ventured_net_status_t s_net;
static ventured_rec_status_t s_rec;
static bool s_has_rec;
static ventured_asr_status_t s_asr;
static bool s_has_asr;

static const char *utf8_next(const char *text, uint32_t *codepoint) {
    const unsigned char *bytes = (const unsigned char *)text;
    if (bytes[0] == 0) {
        *codepoint = 0;
        return text;
    }
    if (bytes[0] < 0x80) {
        *codepoint = bytes[0];
        return text + 1;
    }
    if ((bytes[0] & 0xE0) == 0xC0 && (bytes[1] & 0xC0) == 0x80) {
        *codepoint = ((uint32_t)(bytes[0] & 0x1F) << 6) | (uint32_t)(bytes[1] & 0x3F);
        return text + 2;
    }
    if ((bytes[0] & 0xF0) == 0xE0 && (bytes[1] & 0xC0) == 0x80 && (bytes[2] & 0xC0) == 0x80) {
        *codepoint = ((uint32_t)(bytes[0] & 0x0F) << 12) | ((uint32_t)(bytes[1] & 0x3F) << 6) |
                     (uint32_t)(bytes[2] & 0x3F);
        return text + 3;
    }
    *codepoint = '?';
    return text + 1;
}

static void flush_lcd(void) {
    if (s_io) {
        esp_lcd_panel_io_tx_param(s_io, LCD_CMD_NOP, NULL, 0);
    }
}

static void fill_rect(int x, int y, int w, int h, uint16_t color) {
    if (!s_ready || w <= 0 || h <= 0) return;
    if (w > LCD_HRES) w = LCD_HRES;
    for (int i = 0; i < w; ++i) s_line[i] = color;
    for (int row = 0; row < h; ++row) {
        esp_lcd_panel_draw_bitmap(s_panel, x, y + row, x + w, y + row + 1, s_line);
    }
    flush_lcd();
}

static void draw_glyph(int x, int y, uint32_t codepoint, uint16_t color) {
    const uint32_t *rows = NULL;
    if (!s_ready || !ventured_font_bits(codepoint, &rows)) return;
    for (int row = 0; row < VENTURED_FONT_ROWS; ++row) {
        for (int col = 0; col < VENTURED_FONT_COLS; ++col) {
            s_tile[row * VENTURED_FONT_COLS + col] =
                (rows[row] & (1u << (31 - col))) ? color : COLOR_BG;
        }
    }
    esp_lcd_panel_draw_bitmap(s_panel, x, y, x + VENTURED_FONT_COLS, y + VENTURED_FONT_ROWS, s_tile);
    flush_lcd();
}

static void draw_text(int x, int y, const char *text, uint16_t color) {
    int cursor = x;
    while (text && *text && cursor + VENTURED_FONT_COLS <= LCD_HRES) {
        uint32_t codepoint = 0;
        text = utf8_next(text, &codepoint);
        if (codepoint == 0) break;
        draw_glyph(cursor, y, codepoint, color);
        cursor += VENTURED_FONT_COLS;
    }
}

esp_err_t ventured_display_start(void) {
    spi_bus_config_t bus = {
        .sclk_io_num = BOARD_C_LCD_SCL_GPIO,
        .mosi_io_num = BOARD_C_LCD_SDA_GPIO,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_HRES * 40 * sizeof(uint16_t),
    };
    esp_err_t err = spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "lcd spi bus unavailable");
        return err;
    }
    esp_lcd_panel_io_handle_t io = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = BOARD_C_LCD_DC_GPIO,
        .cs_gpio_num = BOARD_C_LCD_CS_GPIO,
        .pclk_hz = 20 * 1000 * 1000,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 8,
    };
    err = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &io_config, &io);
    if (err != ESP_OK) return err;
    s_io = io;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = -1,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .data_endian = LCD_RGB_DATA_ENDIAN_LITTLE,
        .bits_per_pixel = 16,
    };
    err = esp_lcd_new_panel_st7789(io, &panel_config, &s_panel);
    if (err != ESP_OK) return err;
    esp_lcd_panel_reset(s_panel);
    err = esp_lcd_panel_init(s_panel);
    if (err != ESP_OK) return err;
    esp_lcd_panel_invert_color(s_panel, true);
    esp_lcd_panel_set_gap(s_panel, 0, 0);
    esp_lcd_panel_disp_on_off(s_panel, true);
    if (s_lock == NULL) {
        s_lock = xSemaphoreCreateMutex();
        if (s_lock == NULL) return ESP_ERR_NO_MEM;
    }
    s_ready = true;
    fill_rect(0, 0, LCD_HRES, LCD_VRES, COLOR_BG);
    ventured_net_status_t initial = {0};
    ventured_display_show_net(&initial);
    ESP_LOGI(TAG, "status display ready");
    return ESP_OK;
}

static void redraw_locked(void) {
    fill_rect(0, 0, LCD_HRES, LCD_VRES, COLOR_BG);
    draw_text(16, 16, "网络", COLOR_TITLE);
    uint16_t color = COLOR_MUTED;
    if (s_net.state == VENTURED_NET_CONNECTED) color = COLOR_OK;
    else if (s_net.state == VENTURED_NET_CONNECTING) color = COLOR_WAIT;
    else if (s_net.state == VENTURED_NET_FAILED) color = COLOR_BAD;
    draw_text(16, 64, ventured_net_state_zh(s_net.state), color);
    if (s_net.ip[0] != '\0') {
        draw_text(16, 112, s_net.ip, COLOR_TITLE);
    }
    if (s_has_asr) {
        uint16_t asr_color = COLOR_WAIT;
        if (s_asr.state == VENTURED_ASR_DONE) asr_color = COLOR_OK;
        else if (s_asr.state == VENTURED_ASR_FAIL) asr_color = COLOR_BAD;
        draw_text(16, 176, ventured_asr_state_zh(s_asr.state), asr_color);
        return;
    }
    if (!s_has_rec) return;
    uint16_t rec_color = COLOR_MUTED;
    if (s_rec.state == VENTURED_REC_ACTIVE || s_rec.state == VENTURED_REC_START) rec_color = COLOR_WAIT;
    else if (s_rec.state == VENTURED_REC_DONE && !s_rec.silence) rec_color = COLOR_OK;
    else if (s_rec.state == VENTURED_REC_FAIL || s_rec.silence) rec_color = COLOR_BAD;
    char rec_line[24];
    snprintf(rec_line, sizeof(rec_line), "%s %lu", ventured_rec_state_zh(s_rec.state),
             (unsigned long)s_rec.rms);
    draw_text(16, 176, rec_line, rec_color);
}

static void lock_display(void) {
    if (s_lock) xSemaphoreTake(s_lock, portMAX_DELAY);
}

static void unlock_display(void) {
    if (s_lock) xSemaphoreGive(s_lock);
}

void ventured_display_show_net(const ventured_net_status_t *status) {
    if (!s_ready || status == NULL) return;
    lock_display();
    s_net = *status;
    redraw_locked();
    unlock_display();
}

void ventured_display_show_rec(const ventured_rec_status_t *status) {
    if (!s_ready || status == NULL) return;
    lock_display();
    s_rec = *status;
    s_has_rec = true;
    s_has_asr = false;
    redraw_locked();
    unlock_display();
}

void ventured_display_show_asr(const ventured_asr_status_t *status) {
    if (!s_ready || status == NULL) return;
    lock_display();
    s_asr = *status;
    s_has_asr = true;
    redraw_locked();
    unlock_display();
}
