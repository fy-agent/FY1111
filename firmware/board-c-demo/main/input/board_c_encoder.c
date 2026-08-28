#include "board_c_encoder.h"

#include <limits.h>
#include <stdio.h>

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "board_c_pins.h"
#include "ec11_core.h"
#include "input_event.h"

static const char *const TAG = "board_c_encoder";
#define SCAN_PERIOD_MS 10U
#define BUTTON_DEBOUNCE_MS 25U

static QueueHandle_t s_samples;
static ventured_ec11_t s_encoder;
static ventured_button_t s_action_button;
static ventured_button_t s_button_a;
static ventured_button_t s_button_b;
static uint32_t s_sequence;
static volatile uint32_t s_overflow_count;

static uint8_t IRAM_ATTR read_ab(void) {
    return (uint8_t)((gpio_get_level(BOARD_C_ENCODER_CLK_GPIO) ? 1U : 0U) |
                     (gpio_get_level(BOARD_C_ENCODER_DT_GPIO) ? 2U : 0U));
}

static void IRAM_ATTR encoder_isr(void *unused) {
    (void)unused;
    uint8_t sample = read_ab();
    BaseType_t woke = pdFALSE;
    if (xQueueSendFromISR(s_samples, &sample, &woke) != pdTRUE &&
        s_overflow_count < UINT32_MAX) {
        ++s_overflow_count;
    }
    if (woke == pdTRUE) portYIELD_FROM_ISR();
}

static void emit(ventured_input_t event) {
    char record[80];
    if (!ventured_format_input_event(record, sizeof(record), ++s_sequence, event)) return;
    fputs(record, stdout);
    fflush(stdout);
}

static void encoder_worker(void *unused) {
    (void)unused;
    TickType_t next_button_scan = xTaskGetTickCount();
    for (;;) {
        TickType_t now = xTaskGetTickCount();
        TickType_t wait_ticks =
            (int32_t)(now - next_button_scan) < 0 ? next_button_scan - now : 0;
        uint8_t sample;
        if (xQueueReceive(s_samples, &sample, wait_ticks) == pdTRUE) {
            ventured_input_t event;
            if (ventured_ec11_update(&s_encoder, sample, &event)) emit(event);
        }
        now = xTaskGetTickCount();
        if ((int32_t)(now - next_button_scan) < 0) continue;
        if (ventured_button_update(&s_action_button, gpio_get_level(BOARD_C_ACTION_BUTTON_GPIO) == 0) ==
            VENTURED_BUTTON_STABLE_PRESSED) {
            emit(VENTURED_INPUT_ENCODER_PRESS);
        }
        if (ventured_button_update(&s_button_a, gpio_get_level(BOARD_C_BUTTON_A_GPIO) == 0) ==
            VENTURED_BUTTON_STABLE_PRESSED) {
            emit(VENTURED_INPUT_BUTTON_A);
        }
        if (ventured_button_update(&s_button_b, gpio_get_level(BOARD_C_BUTTON_B_GPIO) == 0) ==
            VENTURED_BUTTON_STABLE_PRESSED) {
            emit(VENTURED_INPUT_BUTTON_B);
        }
        next_button_scan += pdMS_TO_TICKS(SCAN_PERIOD_MS);
        if ((int32_t)(now - next_button_scan) >= 0) {
            next_button_scan = now + pdMS_TO_TICKS(SCAN_PERIOD_MS);
        }
    }
}

static esp_err_t configure_gpio(void) {
    gpio_config_t encoder_io = {
        .pin_bit_mask = (1ULL << BOARD_C_ENCODER_CLK_GPIO) |
                        (1ULL << BOARD_C_ENCODER_DT_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config_t button_io = {
        .pin_bit_mask = (1ULL << BOARD_C_ACTION_BUTTON_GPIO) |
                        (1ULL << BOARD_C_BUTTON_A_GPIO) |
                        (1ULL << BOARD_C_BUTTON_B_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&encoder_io), TAG, "configure encoder GPIO");
    ESP_RETURN_ON_ERROR(gpio_config(&button_io), TAG, "configure action button GPIO");
    return ESP_OK;
}

esp_err_t board_c_encoder_start(void) {
    ESP_RETURN_ON_ERROR(configure_gpio(), TAG, "configure GPIO");
    s_samples = xQueueCreate(32, sizeof(uint8_t));
    if (s_samples == NULL) return ESP_ERR_NO_MEM;
#ifdef CONFIG_VENTURED_ENCODER_INVERT_DIRECTION
    const bool invert_direction = true;
#else
    const bool invert_direction = false;
#endif
    ventured_ec11_init(&s_encoder, read_ab(), invert_direction);
    uint16_t debounce = ventured_button_required_samples(SCAN_PERIOD_MS, BUTTON_DEBOUNCE_MS);
    ventured_button_init(&s_action_button, gpio_get_level(BOARD_C_ACTION_BUTTON_GPIO) == 0, debounce);
    ventured_button_init(&s_button_a, gpio_get_level(BOARD_C_BUTTON_A_GPIO) == 0, debounce);
    ventured_button_init(&s_button_b, gpio_get_level(BOARD_C_BUTTON_B_GPIO) == 0, debounce);
    ESP_RETURN_ON_ERROR(gpio_install_isr_service(0), TAG, "install GPIO ISR");
    ESP_RETURN_ON_ERROR(gpio_isr_handler_add(BOARD_C_ENCODER_CLK_GPIO, encoder_isr, NULL),
                        TAG, "attach CLK ISR");
    ESP_RETURN_ON_ERROR(gpio_isr_handler_add(BOARD_C_ENCODER_DT_GPIO, encoder_isr, NULL),
                        TAG, "attach DT ISR");
    ESP_RETURN_ON_ERROR(gpio_set_intr_type(BOARD_C_ENCODER_CLK_GPIO, GPIO_INTR_ANYEDGE),
                        TAG, "CLK ANYEDGE");
    ESP_RETURN_ON_ERROR(gpio_set_intr_type(BOARD_C_ENCODER_DT_GPIO, GPIO_INTR_ANYEDGE),
                        TAG, "DT ANYEDGE");
    return xTaskCreate(encoder_worker, "encoder_worker", 3072, NULL, 5, NULL) == pdPASS
               ? ESP_OK
               : ESP_ERR_NO_MEM;
}
