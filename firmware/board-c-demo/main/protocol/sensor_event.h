#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    VENTURED_SENSOR_OK = 0,
    VENTURED_SENSOR_TOF,
    VENTURED_SENSOR_I2C,
} ventured_sensor_state_t;

typedef struct {
    uint32_t sequence;
    bool pir;
    bool has_distance;
    uint16_t dist_mm;
    ventured_sensor_state_t state;
} ventured_sensor_status_t;

const char *ventured_sensor_state_token(ventured_sensor_state_t state);
bool ventured_format_sensor_event(char *buffer, size_t capacity, const ventured_sensor_status_t *status);
