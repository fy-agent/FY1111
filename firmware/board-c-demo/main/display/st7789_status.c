#include "st7789_status.h"

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
#include "freertos/timers.h"

#include "audio/pcm_metrics.h"
#include "board_c_pins.h"
#include "font.h"
#include "protocol/asr_event.h"
#include "status_text.h"

#define LCD_HRES 240
#define LCD_VRES 240
#define HEADER_H 40
#define FOOTER_Y 186
#define FOOTER_H (LCD_VRES - FOOTER_Y)
#define ICON_Y 48
#define TITLE_Y 122
#define METER_BARS 6
#define STRIP_ROWS 8
#define TRANSIENT_HOLD_MS 3000

#define COLOR_BG 0x10A4
#define COLOR_TEXT 0xFFFF
#define COLOR_MUTED 0x8C71
#define COLOR_ACCENT 0x07FF
#define COLOR_OK 0x07E0
#define COLOR_WAIT 0xFE60
#define COLOR_BAD 0xF800
#define COLOR_BAR_BG 0x2945
#define COLOR_BAR_FG 0x07E0

typedef enum {
    UI_STATE_OFFLINE = 0,
    UI_STATE_CONNECTING,
    UI_STATE_READY,
    UI_STATE_RECORDING,
    UI_STATE_TRANSCRIBING,
    UI_STATE_THINKING,
    UI_STATE_EXECUTING,
    UI_STATE_SUCCESS,
    UI_STATE_ERROR,
} ui_state_t;

static const char *TAG = "board_c_lcd";
static esp_lcd_panel_handle_t s_panel;
static esp_lcd_panel_io_handle_t s_io;
static uint16_t s_line[LCD_HRES];
static uint16_t s_strip[LCD_HRES * STRIP_ROWS];
static uint16_t s_tile[VENTURED_FONT_ROWS * VENTURED_FONT_COLS];
static bool s_ready;
static SemaphoreHandle_t s_lock;
static ventured_net_status_t s_net;
static ventured_rec_status_t s_rec;
static ventured_asr_status_t s_asr;
static bool s_has_rec;
static bool s_has_asr;
static ui_state_t s_ui;
static uint8_t s_bars = 0xFF;
static TimerHandle_t s_success_timer;

/* 5x7, bit4 = leftmost pixel. Covers space, 0-9, A-Z, . : - */
static const uint8_t k_digit[10][7] = {
    {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E},
    {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E},
    {0x0E, 0x11, 0x01, 0x06, 0x08, 0x10, 0x1F},
    {0x0E, 0x11, 0x01, 0x06, 0x01, 0x11, 0x0E},
    {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02},
    {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E},
    {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E},
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08},
    {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E},
    {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C},
};
static const uint8_t k_alpha[26][7] = {
    {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},
    {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E},
    {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E},
    {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E},
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F},
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10},
    {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0E},
    {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},
    {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E},
    {0x01, 0x01, 0x01, 0x01, 0x11, 0x11, 0x0E},
    {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11},
    {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F},
    {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11},
    {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11},
    {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},
    {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10},
    {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D},
    {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11},
    {0x0E, 0x11, 0x10, 0x0E, 0x01, 0x11, 0x0E},
    {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04},
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04},
    {0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11},
    {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11},
    {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04},
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F},
};

static void flush_lcd(void) {
    if (s_io) {
        esp_lcd_panel_io_tx_param(s_io, LCD_CMD_NOP, NULL, 0);
    }
}

static bool clip_rect(int *x, int *y, int *w, int *h) {
    if (*w <= 0 || *h <= 0) return false;
    if (*x < 0) {
        *w += *x;
        *x = 0;
    }
    if (*y < 0) {
        *h += *y;
        *y = 0;
    }
    if (*x + *w > LCD_HRES) *w = LCD_HRES - *x;
    if (*y + *h > LCD_VRES) *h = LCD_VRES - *y;
    return *w > 0 && *h > 0;
}

static void blit_span(int x, int y, int w, uint16_t color) {
    int h = 1;
    if (!s_ready || !clip_rect(&x, &y, &w, &h)) return;
    for (int i = 0; i < w; ++i) s_line[i] = color;
    esp_lcd_panel_draw_bitmap(s_panel, x, y, x + w, y + 1, s_line);
}

static void fill_rect(int x, int y, int w, int h, uint16_t color) {
    if (!s_ready || !clip_rect(&x, &y, &w, &h)) return;
    int packed = w * STRIP_ROWS;
    for (int i = 0; i < packed; ++i) s_strip[i] = color;
    int row = 0;
    while (row < h) {
        int chunk = h - row;
        if (chunk > STRIP_ROWS) chunk = STRIP_ROWS;
        esp_lcd_panel_draw_bitmap(s_panel, x, y + row, x + w, y + row + chunk, s_strip);
        row += chunk;
    }
    flush_lcd();
}

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

static int glyph_count(const char *text) {
    int count = 0;
    while (text && *text) {
        uint32_t codepoint = 0;
        text = utf8_next(text, &codepoint);
        if (codepoint == 0) break;
        ++count;
    }
    return count;
}

static void draw_glyph(int x, int y, uint32_t codepoint, uint16_t color) {
    const uint32_t *rows = NULL;
    if (!s_ready || !ventured_font_bits(codepoint, &rows)) return;
    if (x < 0 || y < 0 || x + VENTURED_FONT_COLS > LCD_HRES || y + VENTURED_FONT_ROWS > LCD_VRES) {
        return;
    }
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

static void draw_text_centered(int y, const char *text, uint16_t color) {
    int width = glyph_count(text) * VENTURED_FONT_COLS;
    int x = (LCD_HRES - width) / 2;
    if (x < 0) x = 0;
    draw_text(x, y, text, color);
}

static const uint8_t *ascii5x7(char ch) {
    if (ch >= 'a' && ch <= 'z') ch = (char)(ch - 'a' + 'A');
    if (ch >= '0' && ch <= '9') return k_digit[ch - '0'];
    if (ch >= 'A' && ch <= 'Z') return k_alpha[ch - 'A'];
    return NULL;
}

static void draw_ascii_char(int x, int y, char ch, uint16_t color, int scale) {
    const uint8_t *rows = ascii5x7(ch);
    uint8_t special[7];
    if (rows == NULL) {
        memset(special, 0, sizeof(special));
        if (ch == '.') special[5] = 0x04;
        else if (ch == ':') {
            special[1] = 0x04;
            special[5] = 0x04;
        } else if (ch == '-') special[3] = 0x0E;
        else if (ch != ' ') return;
        rows = special;
    }
    if (scale < 1) scale = 1;
    int width = 5 * scale;
    if (width > LCD_HRES) width = LCD_HRES;
    for (int row = 0; row < 7; ++row) {
        for (int i = 0; i < width; ++i) s_line[i] = COLOR_BG;
        for (int col = 0; col < 5; ++col) {
            if ((rows[row] & (0x10 >> col)) == 0) continue;
            for (int sx = 0; sx < scale; ++sx) {
                int px = col * scale + sx;
                if (px < width) s_line[px] = color;
            }
        }
        for (int sy = 0; sy < scale; ++sy) {
            int py = y + row * scale + sy;
            int px = x;
            int pw = width;
            int ph = 1;
            if (!clip_rect(&px, &py, &pw, &ph)) continue;
            esp_lcd_panel_draw_bitmap(s_panel, px, py, px + pw, py + 1, s_line + (px - x));
        }
    }
    flush_lcd();
}

static void draw_ascii_centered(int y, const char *text, uint16_t color, int scale) {
    if (text == NULL || text[0] == '\0') return;
    int advance = 6 * scale;
    int width = (int)strlen(text) * advance;
    int x = (LCD_HRES - width) / 2;
    if (x < 0) x = 4;
    while (*text && x + 5 * scale <= LCD_HRES) {
        draw_ascii_char(x, y, *text++, color, scale);
        x += advance;
    }
}

static void fill_circle(int cx, int cy, int r, uint16_t color) {
    if (r <= 0) return;
    for (int y = -r; y <= r; ++y) {
        int yy = y * y;
        int x = 0;
        while (x * x + yy <= r * r) ++x;
        int w = x * 2 - 1;
        if (w > 0) blit_span(cx - x + 1, cy + y, w, color);
    }
    flush_lcd();
}

static void draw_line(int x0, int y0, int x1, int y1, uint16_t color, int thick) {
    int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int sx = x0 < x1 ? 1 : -1;
    int dy = y1 > y0 ? y0 - y1 : y1 - y0;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    if (thick < 1) thick = 1;
    while (s_ready) {
        fill_rect(x0 - thick / 2, y0 - thick / 2, thick, thick, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = err * 2;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

static uint16_t net_dot_color(void) {
    if (s_net.state == VENTURED_NET_CONNECTED) return COLOR_OK;
    if (s_net.state == VENTURED_NET_CONNECTING) return COLOR_WAIT;
    if (s_net.state == VENTURED_NET_FAILED) return COLOR_BAD;
    return COLOR_MUTED;
}

static uint8_t rssi_bars(void) {
    if (s_net.state != VENTURED_NET_CONNECTED) return 0;
    if (s_net.rssi >= -50) return 4;
    if (s_net.rssi >= -60) return 3;
    if (s_net.rssi >= -70) return 2;
    if (s_net.rssi >= -80) return 1;
    return 0;
}

static uint8_t rms_to_bars(uint32_t rms) {
    if (rms < (uint32_t)VENTURED_PCM_SILENCE_RMS) return 0;
    if (rms < 160) return 1;
    if (rms < 320) return 2;
    if (rms < 640) return 3;
    if (rms < 1280) return 4;
    if (rms < 2560) return 5;
    return 6;
}

static ui_state_t ui_from_net(void) {
    if (s_net.state == VENTURED_NET_CONNECTED) return UI_STATE_READY;
    if (s_net.state == VENTURED_NET_CONNECTING) return UI_STATE_CONNECTING;
    return UI_STATE_OFFLINE;
}

static bool ui_holds_scene(ui_state_t state) {
    return state == UI_STATE_RECORDING || state == UI_STATE_TRANSCRIBING ||
           state == UI_STATE_SUCCESS || state == UI_STATE_ERROR;
}

static bool ui_is_transient(ui_state_t state) {
    return state == UI_STATE_SUCCESS || state == UI_STATE_ERROR;
}

static void draw_wifi_bars(int x, int y) {
    uint8_t lit = rssi_bars();
    const int heights[4] = {8, 12, 16, 20};
    for (int i = 0; i < 4; ++i) {
        int h = heights[i];
        uint16_t color = (i < lit) ? COLOR_OK : COLOR_BAR_BG;
        if (s_net.state == VENTURED_NET_CONNECTING && i == 0) color = COLOR_WAIT;
        fill_rect(x + i * 8, y + (20 - h), 5, h, color);
    }
}

static void draw_header(void) {
    fill_rect(0, 0, LCD_HRES, HEADER_H, COLOR_BG);
    fill_circle(18, 20, 5, net_dot_color());
    draw_text(32, 4, ventured_net_state_zh(s_net.state), COLOR_TEXT);
    draw_wifi_bars(200, 10);
}

static void draw_mic_icon(uint16_t color) {
    int cx = LCD_HRES / 2;
    int top = ICON_Y;
    fill_rect(cx - 14, top + 8, 28, 30, color);
    fill_circle(cx, top + 12, 14, color);
    fill_circle(cx, top + 34, 14, color);
    fill_rect(cx - 3, top + 46, 6, 8, color);
    fill_rect(cx - 16, top + 54, 32, 5, color);
    fill_rect(cx - 22, top + 18, 4, 22, color);
    fill_rect(cx + 18, top + 18, 4, 22, color);
}

static void draw_check_icon(void) {
    int cx = LCD_HRES / 2;
    int cy = ICON_Y + 32;
    draw_line(cx - 22, cy, cx - 6, cy + 18, COLOR_OK, 5);
    draw_line(cx - 6, cy + 18, cx + 26, cy - 16, COLOR_OK, 5);
}

static void draw_cross_icon(void) {
    int cx = LCD_HRES / 2;
    int cy = ICON_Y + 32;
    draw_line(cx - 20, cy - 18, cx + 20, cy + 18, COLOR_BAD, 5);
    draw_line(cx + 20, cy - 18, cx - 20, cy + 18, COLOR_BAD, 5);
}

static void draw_process_icon(void) {
    int cx = LCD_HRES / 2;
    int cy = ICON_Y + 32;
    fill_circle(cx - 22, cy, 7, COLOR_WAIT);
    fill_circle(cx, cy, 7, COLOR_ACCENT);
    fill_circle(cx + 22, cy, 7, COLOR_WAIT);
}

static void draw_meter(uint8_t bars) {
    fill_rect(0, FOOTER_Y, LCD_HRES, FOOTER_H, COLOR_BG);
    const int gap = 6;
    const int bar_w = 26;
    const int total = METER_BARS * bar_w + (METER_BARS - 1) * gap;
    int x = (LCD_HRES - total) / 2;
    const int heights[METER_BARS] = {10, 16, 22, 28, 34, 40};
    int base = LCD_VRES - 8;
    for (int i = 0; i < METER_BARS; ++i) {
        int h = heights[i];
        int by = base - h;
        fill_rect(x, by, bar_w, h, COLOR_BAR_BG);
        if (i < bars) {
            uint16_t color = (i >= 4) ? COLOR_OK : COLOR_ACCENT;
            fill_rect(x + 2, by + 2, bar_w - 4, h - 4, color);
        }
        x += bar_w + gap;
    }
}

static void draw_footer(void) {
    fill_rect(0, FOOTER_Y, LCD_HRES, FOOTER_H, COLOR_BG);
    if (s_ui == UI_STATE_RECORDING) {
        s_bars = rms_to_bars(s_rec.rms);
        draw_meter(s_bars);
        return;
    }
    if (s_ui == UI_STATE_READY && s_net.ip[0] != '\0') {
        draw_ascii_centered(FOOTER_Y + 18, s_net.ip, COLOR_MUTED, 2);
        return;
    }
    if (s_ui == UI_STATE_ERROR) {
        const char *reason = s_has_asr && s_asr.reason[0] != '\0' ? s_asr.reason : s_rec.reason;
        if (reason[0] != '\0') {
            draw_ascii_centered(FOOTER_Y + 20, reason, COLOR_BAD, 2);
        }
    }
}

static const char *hero_title(void) {
    switch (s_ui) {
        case UI_STATE_READY:
            return "可录音";
        case UI_STATE_RECORDING:
            return "录音中";
        case UI_STATE_TRANSCRIBING:
        case UI_STATE_THINKING:
        case UI_STATE_EXECUTING:
            return "转写";
        case UI_STATE_SUCCESS:
            return s_has_asr ? "转写完成" : "完成";
        case UI_STATE_ERROR:
            return s_has_asr ? "转写失败" : "失败";
        case UI_STATE_CONNECTING:
            return "连接中";
        case UI_STATE_OFFLINE:
        default:
            return ventured_net_state_zh(s_net.state);
    }
}

static uint16_t hero_title_color(void) {
    switch (s_ui) {
        case UI_STATE_READY:
            return COLOR_TEXT;
        case UI_STATE_RECORDING:
        case UI_STATE_TRANSCRIBING:
        case UI_STATE_CONNECTING:
            return COLOR_WAIT;
        case UI_STATE_SUCCESS:
            return COLOR_OK;
        case UI_STATE_ERROR:
            return COLOR_BAD;
        default:
            return COLOR_MUTED;
    }
}

static void draw_hero(void) {
    fill_rect(0, HEADER_H, LCD_HRES, FOOTER_Y - HEADER_H, COLOR_BG);
    switch (s_ui) {
        case UI_STATE_RECORDING:
            draw_mic_icon(COLOR_WAIT);
            break;
        case UI_STATE_READY:
            draw_mic_icon(COLOR_ACCENT);
            break;
        case UI_STATE_TRANSCRIBING:
        case UI_STATE_THINKING:
        case UI_STATE_EXECUTING:
            draw_process_icon();
            break;
        case UI_STATE_SUCCESS:
            draw_check_icon();
            break;
        case UI_STATE_ERROR:
            draw_cross_icon();
            break;
        default:
            draw_mic_icon(COLOR_MUTED);
            break;
    }
    draw_text_centered(TITLE_Y, hero_title(), hero_title_color());
}

static void redraw_full(void) {
    fill_rect(0, 0, LCD_HRES, LCD_VRES, COLOR_BG);
    draw_header();
    draw_hero();
    draw_footer();
}

static void lock_display(void) {
    if (s_lock) xSemaphoreTake(s_lock, portMAX_DELAY);
}

static void unlock_display(void) {
    if (s_lock) xSemaphoreGive(s_lock);
}

static void cancel_success_hold(void) {
    if (s_success_timer) {
        xTimerStop(s_success_timer, 0);
    }
}

static void arm_success_hold(void) {
    if (s_success_timer == NULL) return;
    xTimerReset(s_success_timer, 0);
}

static void set_ui_state(ui_state_t next) {
    bool already = s_ui == next;
    s_ui = next;
    if (ui_is_transient(next)) {
        if (!already || next == UI_STATE_SUCCESS) arm_success_hold();
    } else {
        cancel_success_hold();
    }
}

static void success_hold_cb(TimerHandle_t timer) {
    (void)timer;
    lock_display();
    if (s_ready && ui_is_transient(s_ui)) {
        s_ui = ui_from_net();
        s_has_rec = false;
        s_has_asr = false;
        s_bars = 0xFF;
        redraw_full();
    }
    unlock_display();
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
    if (s_success_timer == NULL) {
        s_success_timer = xTimerCreate("lcd_hold", pdMS_TO_TICKS(TRANSIENT_HOLD_MS), pdFALSE, NULL, success_hold_cb);
        if (s_success_timer == NULL) return ESP_ERR_NO_MEM;
    }
    s_ready = true;
    s_ui = UI_STATE_OFFLINE;
    s_bars = 0xFF;
    ventured_net_status_t initial = {0};
    ventured_display_show_net(&initial);
    ESP_LOGI(TAG, "status display ready");
    return ESP_OK;
}

void ventured_display_show_net(const ventured_net_status_t *status) {
    if (!s_ready || status == NULL) return;
    lock_display();
    s_net = *status;
    if (ui_holds_scene(s_ui)) {
        draw_header();
    } else {
        ui_state_t next = ui_from_net();
        if (next != s_ui) {
            set_ui_state(next);
            s_has_rec = false;
            s_has_asr = false;
            redraw_full();
        } else {
            draw_header();
            if (s_ui == UI_STATE_READY) {
                fill_rect(0, FOOTER_Y, LCD_HRES, FOOTER_H, COLOR_BG);
                if (s_net.ip[0] != '\0') {
                    draw_ascii_centered(FOOTER_Y + 18, s_net.ip, COLOR_MUTED, 2);
                }
            }
        }
    }
    unlock_display();
}

void ventured_display_show_rec(const ventured_rec_status_t *status) {
    if (!s_ready || status == NULL) return;
    lock_display();
    s_rec = *status;
    s_has_rec = true;
    s_has_asr = false;
    ui_state_t next = UI_STATE_RECORDING;
    if (status->state == VENTURED_REC_FAIL) {
        next = UI_STATE_ERROR;
    } else if (status->state == VENTURED_REC_DONE) {
        next = status->silence ? ui_from_net() : UI_STATE_SUCCESS;
        if (status->silence) s_has_rec = false;
    }
    if (next == UI_STATE_ERROR && s_ui == UI_STATE_ERROR) {
        unlock_display();
        return;
    }
    if (next == UI_STATE_RECORDING && s_ui == UI_STATE_RECORDING) {
        uint8_t bars = rms_to_bars(status->rms);
        if (bars != s_bars) {
            s_bars = bars;
            draw_meter(bars);
        }
    } else {
        set_ui_state(next);
        s_bars = 0xFF;
        redraw_full();
    }
    unlock_display();
}

void ventured_display_show_asr(const ventured_asr_status_t *status) {
    if (!s_ready || status == NULL) return;
    lock_display();
    if (status->state == VENTURED_ASR_FAIL && strcmp(status->reason, "CANCEL") == 0) {
        s_has_asr = false;
        s_has_rec = false;
        set_ui_state(ui_from_net());
        s_bars = 0xFF;
        redraw_full();
        unlock_display();
        return;
    }
    s_asr = *status;
    s_has_asr = true;
    ui_state_t next = UI_STATE_TRANSCRIBING;
    if (status->state == VENTURED_ASR_DONE) next = UI_STATE_SUCCESS;
    else if (status->state == VENTURED_ASR_FAIL) next = UI_STATE_ERROR;
    if (next == UI_STATE_ERROR && s_ui == UI_STATE_ERROR) {
        unlock_display();
        return;
    }
    set_ui_state(next);
    s_bars = 0xFF;
    redraw_full();
    unlock_display();
}
