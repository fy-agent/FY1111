#pragma once

/* Board C pin manifest: do not allocate a reserved pin in this MVP. */
enum {
    BOARD_C_ENCODER_CLK_GPIO = 6,
    BOARD_C_ENCODER_DT_GPIO = 7,
    BOARD_C_ACTION_BUTTON_GPIO = 8,
    BOARD_C_MIC_BUTTON_GPIO = 9,
    BOARD_C_BUTTON_A_GPIO = 10,
    BOARD_C_BUTTON_B_GPIO = 11,
    BOARD_C_MIC_WS_GPIO = 42,
    BOARD_C_MIC_SD_GPIO = 2,
    BOARD_C_MIC_SCK_GPIO = 41,
    BOARD_C_LCD_SCL_GPIO = 21,
    BOARD_C_LCD_SDA_GPIO = 47,
    BOARD_C_LCD_DC_GPIO = 43,
    BOARD_C_LCD_CS_GPIO = 44,
    BOARD_C_WS2812_GPIO = 48,
    BOARD_C_PTT_GPIO = 12,
    BOARD_C_PIR_GPIO = 16,
    BOARD_C_I2C_SDA_GPIO = 4,
    BOARD_C_I2C_SCL_GPIO = 5,
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
_Static_assert(BOARD_C_BUTTON_A_GPIO != BOARD_C_BUTTON_B_GPIO,
               "extra pull-down buttons must use distinct pins");
_Static_assert(BOARD_C_BUTTON_A_GPIO != BOARD_C_ACTION_BUTTON_GPIO,
               "GPIO10 must not reuse GPIO8");
_Static_assert(BOARD_C_BUTTON_B_GPIO != BOARD_C_ACTION_BUTTON_GPIO,
               "GPIO11 must not reuse GPIO8");
_Static_assert(BOARD_C_BUTTON_A_GPIO != BOARD_C_MIC_BUTTON_GPIO,
               "GPIO10 must not reuse GPIO9");
_Static_assert(BOARD_C_BUTTON_B_GPIO != BOARD_C_MIC_BUTTON_GPIO,
               "GPIO11 must not reuse GPIO9");
_Static_assert(BOARD_C_BUTTON_A_GPIO != BOARD_C_PTT_GPIO,
               "GPIO10 must not reuse reserved GPIO12");
_Static_assert(BOARD_C_BUTTON_B_GPIO != BOARD_C_PTT_GPIO,
               "GPIO11 must not reuse reserved GPIO12");
_Static_assert(BOARD_C_BUTTON_A_GPIO != BOARD_C_ENCODER_CLK_GPIO,
               "GPIO10 must not reuse an encoder pin");
_Static_assert(BOARD_C_BUTTON_A_GPIO != BOARD_C_ENCODER_DT_GPIO,
               "GPIO10 must not reuse an encoder pin");
_Static_assert(BOARD_C_BUTTON_B_GPIO != BOARD_C_ENCODER_CLK_GPIO,
               "GPIO11 must not reuse an encoder pin");
_Static_assert(BOARD_C_BUTTON_B_GPIO != BOARD_C_ENCODER_DT_GPIO,
               "GPIO11 must not reuse an encoder pin");
_Static_assert(BOARD_C_I2C_SDA_GPIO != BOARD_C_I2C_SCL_GPIO,
               "I2C SDA and SCL must be distinct");
_Static_assert(BOARD_C_PIR_GPIO != BOARD_C_I2C_SDA_GPIO,
               "PIR must not reuse I2C SDA");
_Static_assert(BOARD_C_PIR_GPIO != BOARD_C_I2C_SCL_GPIO,
               "PIR must not reuse I2C SCL");
_Static_assert(BOARD_C_PIR_GPIO != BOARD_C_MIC_BUTTON_GPIO,
               "PIR must not reuse GPIO9");
_Static_assert(BOARD_C_PIR_GPIO != BOARD_C_ACTION_BUTTON_GPIO,
               "PIR must not reuse GPIO8");
_Static_assert(BOARD_C_PIR_GPIO != BOARD_C_PTT_GPIO,
               "PIR must not reuse reserved GPIO12");
_Static_assert(BOARD_C_I2C_SDA_GPIO != BOARD_C_ENCODER_CLK_GPIO,
               "I2C must not reuse an encoder pin");
_Static_assert(BOARD_C_I2C_SCL_GPIO != BOARD_C_ENCODER_DT_GPIO,
               "I2C must not reuse an encoder pin");
_Static_assert(BOARD_C_I2C_SDA_GPIO != BOARD_C_MIC_SD_GPIO,
               "I2C SDA must not reuse the microphone data pin");
