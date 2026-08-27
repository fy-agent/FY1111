#include "unity.h"

#include "board_c_pins.h"
#include "ec11_core.h"
#include "input_event.h"

void setUp(void) {}
void tearDown(void) {}

void test_complete_clockwise_detent_emits_once(void) {
    ventured_ec11_t encoder;
    ventured_input_t event = VENTURED_INPUT_ENCODER_PRESS;
    ventured_ec11_init(&encoder, 0, false);
    TEST_ASSERT_FALSE(ventured_ec11_update(&encoder, 1, &event));
    TEST_ASSERT_FALSE(ventured_ec11_update(&encoder, 3, &event));
    TEST_ASSERT_FALSE(ventured_ec11_update(&encoder, 2, &event));
    TEST_ASSERT_TRUE(ventured_ec11_update(&encoder, 0, &event));
    TEST_ASSERT_EQUAL(VENTURED_INPUT_ENCODER_CW, event);
}

void test_complete_counterclockwise_detent_and_inversion(void) {
    ventured_ec11_t encoder;
    ventured_input_t event;
    ventured_ec11_init(&encoder, 0, false);
    TEST_ASSERT_FALSE(ventured_ec11_update(&encoder, 2, &event));
    TEST_ASSERT_FALSE(ventured_ec11_update(&encoder, 3, &event));
    TEST_ASSERT_FALSE(ventured_ec11_update(&encoder, 1, &event));
    TEST_ASSERT_TRUE(ventured_ec11_update(&encoder, 0, &event));
    TEST_ASSERT_EQUAL(VENTURED_INPUT_ENCODER_CCW, event);

    ventured_ec11_init(&encoder, 0, true);
    TEST_ASSERT_FALSE(ventured_ec11_update(&encoder, 2, &event));
    TEST_ASSERT_FALSE(ventured_ec11_update(&encoder, 3, &event));
    TEST_ASSERT_FALSE(ventured_ec11_update(&encoder, 1, &event));
    TEST_ASSERT_TRUE(ventured_ec11_update(&encoder, 0, &event));
    TEST_ASSERT_EQUAL(VENTURED_INPUT_ENCODER_CW, event);
}

void test_bounce_and_reversal_do_not_invent_a_detent(void) {
    ventured_ec11_t encoder;
    ventured_input_t event;
    ventured_ec11_init(&encoder, 0, false);
    TEST_ASSERT_FALSE(ventured_ec11_update(&encoder, 1, &event));
    TEST_ASSERT_FALSE(ventured_ec11_update(&encoder, 1, &event));
    TEST_ASSERT_FALSE(ventured_ec11_update(&encoder, 0, &event));
    TEST_ASSERT_EQUAL_INT8(0, encoder.quarter_steps);
    TEST_ASSERT_FALSE(ventured_ec11_update(&encoder, 2, &event));
    TEST_ASSERT_FALSE(ventured_ec11_update(&encoder, 3, &event));
    TEST_ASSERT_FALSE(ventured_ec11_update(&encoder, 2, &event));
    TEST_ASSERT_FALSE(ventured_ec11_update(&encoder, 0, &event));
    TEST_ASSERT_EQUAL_INT8(0, encoder.quarter_steps);
}

void test_invalid_jump_clears_partial_detent(void) {
    ventured_ec11_t encoder;
    ventured_input_t event;
    ventured_ec11_init(&encoder, 0, false);
    TEST_ASSERT_FALSE(ventured_ec11_update(&encoder, 1, &event));
    TEST_ASSERT_FALSE(ventured_ec11_update(&encoder, 2, &event));
    TEST_ASSERT_EQUAL_UINT32(1, encoder.invalid_transition_count);
    TEST_ASSERT_EQUAL_INT8(0, encoder.quarter_steps);
}

void test_button_requires_four_observations_to_span_25ms_at_10ms(void) {
    ventured_button_t button;
    ventured_button_init(&button, false, ventured_button_required_samples(10, 25));
    TEST_ASSERT_EQUAL_UINT16(4, button.required_samples);
    TEST_ASSERT_EQUAL(VENTURED_BUTTON_NO_CHANGE, ventured_button_update(&button, true));
    TEST_ASSERT_EQUAL(VENTURED_BUTTON_NO_CHANGE, ventured_button_update(&button, true));
    TEST_ASSERT_EQUAL(VENTURED_BUTTON_NO_CHANGE, ventured_button_update(&button, true));
    TEST_ASSERT_EQUAL(VENTURED_BUTTON_STABLE_PRESSED, ventured_button_update(&button, true));
    TEST_ASSERT_EQUAL(VENTURED_BUTTON_NO_CHANGE, ventured_button_update(&button, false));
    TEST_ASSERT_EQUAL(VENTURED_BUTTON_NO_CHANGE, ventured_button_update(&button, false));
    TEST_ASSERT_EQUAL(VENTURED_BUTTON_NO_CHANGE, ventured_button_update(&button, false));
    TEST_ASSERT_EQUAL(VENTURED_BUTTON_STABLE_RELEASED, ventured_button_update(&button, false));
}

void test_event_formatter_is_exact_and_bounded(void) {
    char event[80];
    TEST_ASSERT_TRUE(ventured_format_input_event(event, sizeof(event), 7, VENTURED_INPUT_ENCODER_PRESS));
    TEST_ASSERT_EQUAL_STRING("VKEY_INPUT/1 {\"seq\":7,\"input\":\"ENCODER_PRESS\"}\n", event);
    TEST_ASSERT_FALSE(ventured_format_input_event(event, 4, 7, VENTURED_INPUT_ENCODER_PRESS));
}

void test_board_c_manifest_has_exact_encoder_pins_and_no_listed_collisions(void) {
    const int pins[] = {
        BOARD_C_ENCODER_CLK_GPIO, BOARD_C_ENCODER_DT_GPIO, BOARD_C_ACTION_BUTTON_GPIO,
        BOARD_C_MIC_WS_GPIO, BOARD_C_MIC_SD_GPIO, BOARD_C_MIC_SCK_GPIO,
        BOARD_C_LCD_SCL_GPIO, BOARD_C_LCD_SDA_GPIO, BOARD_C_LCD_DC_GPIO,
        BOARD_C_LCD_CS_GPIO, BOARD_C_WS2812_GPIO, BOARD_C_PTT_GPIO,
    };
    TEST_ASSERT_EQUAL_INT(6, BOARD_C_ENCODER_CLK_GPIO);
    TEST_ASSERT_EQUAL_INT(7, BOARD_C_ENCODER_DT_GPIO);
    TEST_ASSERT_EQUAL_INT(8, BOARD_C_ACTION_BUTTON_GPIO);
    for (unsigned int left = 0; left < sizeof(pins) / sizeof(pins[0]); ++left) {
        for (unsigned int right = left + 1; right < sizeof(pins) / sizeof(pins[0]); ++right) {
            TEST_ASSERT_NOT_EQUAL(pins[left], pins[right]);
        }
    }
}
