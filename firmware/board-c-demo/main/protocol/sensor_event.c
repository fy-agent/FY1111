#include "sensor_event.h"

#include <inttypes.h>
#include <stdio.h>

const char *ventured_sensor_state_token(ventured_sensor_state_t state) {
    switch (state) {
        case VENTURED_SENSOR_OK: return "OK";
        case VENTURED_SENSOR_TOF: return "TOF";
        case VENTURED_SENSOR_I2C: return "I2C";
        default: return NULL;
    }
}

bool ventured_format_sensor_event(char *buffer, size_t capacity, const ventured_sensor_status_t *status) {
    const char *state = status == NULL ? NULL : ventured_sensor_state_token(status->state);
    if (buffer == NULL || capacity == 0U || state == NULL) return false;
    int written = snprintf(buffer, capacity,
                           "VKEY_SENSOR/1 {\"seq\":%" PRIu32 ",\"pir\":%s,\"state\":\"%s\"",
                           status->sequence, status->pir ? "true" : "false", state);
    if (written < 0 || (size_t)written >= capacity) return false;
    size_t used = (size_t)written;
    if (status->has_distance) {
        written = snprintf(buffer + used, capacity - used, ",\"distMm\":%" PRIu16 "}\n", status->dist_mm);
    } else {
        written = snprintf(buffer + used, capacity - used, "}\n");
    }
    return written >= 0 && (size_t)written < capacity - used;
}
