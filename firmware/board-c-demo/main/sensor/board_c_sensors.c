#include "board_c_sensors.h"

#include <stdio.h>

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "vl53l0x.h"

#include "audio/mic_rec.h"
#include "board_c_pins.h"
#include "net/asr_upload.h"
#include "sensor_event.h"
#include "tof_hand_gesture.h"

#define VL53L0X_ADDR 0x29
#define SENSOR_LOOP_MS 50U
#define SENSOR_EMIT_MS 400U
#define TOF_RETRY_MS 5000U
#define SEAT_HOLD_MS 4000U

static const char *TAG = "board_c_sensors";
static i2c_master_bus_handle_t s_bus;
static vl53l0x_handle_t s_tof;
static bool s_tof_continuous;
static uint32_t s_sequence;

static void emit_sensor(const ventured_sensor_status_t *status) {
    char record[128];
    if (!ventured_format_sensor_event(record, sizeof(record), status)) return;
    fputs(record, stdout);
    fflush(stdout);
}

static bool pir_motion(void) {
    return gpio_get_level(BOARD_C_PIR_GPIO) != 0;
}

static bool init_tof(void) {
    if (s_bus == NULL) {
        const i2c_master_bus_config_t bus_cfg = {
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .i2c_port = I2C_NUM_0,
            .sda_io_num = BOARD_C_I2C_SDA_GPIO,
            .scl_io_num = BOARD_C_I2C_SCL_GPIO,
            .glitch_ignore_cnt = 7,
            .flags.enable_internal_pullup = true,
        };
        if (i2c_new_master_bus(&bus_cfg, &s_bus) != ESP_OK) {
            ESP_LOGW(TAG, "i2c bus failed");
            return false;
        }
    }
    if (i2c_master_probe(s_bus, VL53L0X_ADDR, 100) != ESP_OK) {
        ESP_LOGW(TAG, "vl53l0x not found at 0x29");
        return false;
    }
    if (s_tof != NULL) {
        vl53l0x_destroy(s_tof);
        s_tof = NULL;
    }
    if (vl53l0x_create(&s_tof, s_bus) != ESP_OK || vl53l0x_init(s_tof) != ESP_OK) {
        ESP_LOGW(TAG, "vl53l0x init failed");
        if (s_tof != NULL) {
            vl53l0x_destroy(s_tof);
            s_tof = NULL;
        }
        return false;
    }
    vl53l0x_ref_spad_calibration_t spad = {0};
    if (vl53l0x_perform_ref_spad_management(s_tof, &spad) != ESP_OK) {
        ESP_LOGW(TAG, "vl53l0x spad cal skipped");
    }
    vl53l0x_ref_calibration_t ref = {0};
    if (vl53l0x_perform_ref_calibration(s_tof, &ref) != ESP_OK) {
        ESP_LOGW(TAG, "vl53l0x ref cal skipped");
    }
    if (vl53l0x_set_profile(s_tof, VL53L0X_PROFILE_HIGH_SPEED) != ESP_OK &&
        vl53l0x_set_profile(s_tof, VL53L0X_PROFILE_DEFAULT) != ESP_OK) {
        ESP_LOGW(TAG, "vl53l0x profile failed");
        return false;
    }
    s_tof_continuous = false;
    if (vl53l0x_set_mode(s_tof, VL53L0X_MODE_CONTINUOUS_TIMED) == ESP_OK &&
        vl53l0x_set_inter_measurement(s_tof, SENSOR_LOOP_MS) == ESP_OK &&
        vl53l0x_start_measurement(s_tof) == ESP_OK) {
        s_tof_continuous = true;
        ESP_LOGI(TAG, "vl53l0x continuous on GPIO4/GPIO5");
        return true;
    }
    ESP_LOGW(TAG, "vl53l0x continuous unavailable; single-shot fallback");
    return true;
}

static bool read_tof(vl53l0x_data_t *sample) {
    if (s_tof == NULL || sample == NULL) return false;
    if (s_tof_continuous) {
        bool ready = false;
        if (vl53l0x_get_ready(s_tof, &ready) != ESP_OK || !ready) return false;
        return vl53l0x_get_data(s_tof, sample) == ESP_OK && sample->valid;
    }
    return vl53l0x_single_measure(s_tof, sample) == ESP_OK && sample->valid;
}

static void apply_gesture(ventured_hand_gesture_t gesture, bool occupied) {
    if (gesture == VENTURED_HAND_ENTER) {
        bool cancel = ventured_asr_busy();
        if (cancel || occupied) {
            ventured_mic_rec_set_hand_held(true);
            if (cancel) ESP_LOGI(TAG, "hand enter -> asr cancel");
            else ESP_LOGI(TAG, "hand enter -> rec start");
        }
    } else if (gesture == VENTURED_HAND_LEAVE) {
        ventured_mic_rec_set_hand_held(false);
        ESP_LOGI(TAG, "hand leave -> rec stop");
    }
}

static void sensor_task(void *unused) {
    (void)unused;
    bool tof_ok = init_tof();
    uint32_t retry_left = 0;
    uint32_t emit_left = 0;
    int64_t seat_until_us = 0;
    ventured_hand_tracker_t hand;
    ventured_hand_tracker_init(&hand);
    while (true) {
        bool pir = pir_motion();
        int64_t now_us = esp_timer_get_time();
        if (pir) seat_until_us = now_us + (int64_t)SEAT_HOLD_MS * 1000;
        bool occupied = pir || now_us < seat_until_us;

        ventured_sensor_status_t status = {
            .sequence = s_sequence + 1,
            .pir = occupied,
            .state = tof_ok ? VENTURED_SENSOR_OK : (s_bus == NULL ? VENTURED_SENSOR_I2C : VENTURED_SENSOR_TOF),
        };
        bool valid = false;
        uint16_t dist_mm = 0;
        if (tof_ok && s_tof != NULL) {
            vl53l0x_data_t sample = {0};
            if (read_tof(&sample)) {
                valid = true;
                dist_mm = sample.distance_mm;
                status.has_distance = true;
                status.dist_mm = dist_mm;
                status.state = VENTURED_SENSOR_OK;
            } else {
                status.state = VENTURED_SENSOR_TOF;
            }
        } else if (!tof_ok) {
            if (retry_left == 0) {
                tof_ok = init_tof();
                retry_left = TOF_RETRY_MS / SENSOR_LOOP_MS;
            } else {
                --retry_left;
            }
        }

        apply_gesture(ventured_hand_tracker_update(&hand, valid, dist_mm, (uint32_t)(now_us / 1000)),
                      occupied);

        if (emit_left == 0) {
            ++s_sequence;
            status.sequence = s_sequence;
            emit_sensor(&status);
            emit_left = SENSOR_EMIT_MS / SENSOR_LOOP_MS;
        } else {
            --emit_left;
        }
        vTaskDelay(pdMS_TO_TICKS(SENSOR_LOOP_MS));
    }
}

esp_err_t board_c_sensors_start(void) {
    gpio_config_t pir = {
        .pin_bit_mask = 1ULL << BOARD_C_PIR_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&pir);
    if (err != ESP_OK) return err;
    if (xTaskCreate(sensor_task, "board_c_sensors", 8192, NULL, 4, NULL) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}
