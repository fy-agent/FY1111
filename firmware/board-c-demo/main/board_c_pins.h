#pragma once

/* Board C pin manifest: do not allocate a reserved pin in this MVP. */
enum {
    BOARD_C_ENCODER_CLK_GPIO = 6,
    BOARD_C_ENCODER_DT_GPIO = 7,
    BOARD_C_ACTION_BUTTON_GPIO = 8,
    BOARD_C_MIC_BUTTON_GPIO = 9,
    BOARD_C_MIC_WS_GPIO = 42,
    BOARD_C_MIC_SD_GPIO = 2,
    BOARD_C_MIC_SCK_GPIO = 41,
    BOARD_C_LCD_SCL_GPIO = 21,
    BOARD_C_LCD_SDA_GPIO = 47,
    BOARD_C_LCD_DC_GPIO = 43,
    BOARD_C_LCD_CS_GPIO = 44,
    BOARD_C_WS2812_GPIO = 48,
    BOARD_C_PTT_GPIO = 12,
};

_Static_assert(BOARD_C_ENCODER_CLK_GPIO != BOARD_C_ENCODER_DT_GPIO,
               "active encoder pins must be distinct");
_Static_assert(BOARD_C_ENCODER_CLK_GPIO != BOARD_C_ACTION_BUTTON_GPIO,
               "encoder and action button pins must be distinct");
_Static_assert(BOARD_C_ENCODER_DT_GPIO != BOARD_C_ACTION_BUTTON_GPIO,
               "encoder and action button pins must be distinct");
_Static_assert(BOARD_C_ENCODER_CLK_GPIO != BOARD_C_MIC_SD_GPIO,
               "encoder must not reuse the microphone data pin");
_Static_assert(BOARD_C_MIC_BUTTON_GPIO != BOARD_C_ACTION_BUTTON_GPIO,
               "mic start button must not reuse the action button");
_Static_assert(BOARD_C_MIC_BUTTON_GPIO != BOARD_C_ENCODER_CLK_GPIO,
               "mic start button must not reuse an encoder pin");
_Static_assert(BOARD_C_MIC_BUTTON_GPIO != BOARD_C_ENCODER_DT_GPIO,
               "mic start button must not reuse an encoder pin");
_Static_assert(BOARD_C_MIC_BUTTON_GPIO != BOARD_C_MIC_SD_GPIO,
               "mic start button must not reuse the microphone data pin");
_Static_assert(BOARD_C_MIC_BUTTON_GPIO != BOARD_C_PTT_GPIO,
               "mic start button is GPIO9; GPIO12 stays reserved");
