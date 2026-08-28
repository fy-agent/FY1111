#pragma once

#include <stdbool.h>
#include <stdint.h>

/*
 * 1D approach/leave detector for a single VL53L0X.
 * Same idea as ST Proximity_Gesture SWIPE_1 / UM2039 faucet demos:
 * hysteresis bands on range, then far->near vs near->far.
 *
 * Board-C measured bands (sensor facing up):
 *   sky / no hand : about 20-70 mm  -> treated as empty
 *   hand near     : about 80-120 mm
 *   hand far      : about 200 mm or more
 *
 * A zone must dwell before it counts, and ENTER/LEAVE lock out the opposite
 * edge so a withdraw bounce cannot restart recording.
 */

enum {
    VENTURED_HAND_DWELL_MS = 80U,
    VENTURED_HAND_LOCKOUT_MS = 500U,
    VENTURED_HAND_MEMORY_MS = 1200U,
};

typedef enum {
    VENTURED_HAND_NONE = 0,
    VENTURED_HAND_ENTER,
    VENTURED_HAND_LEAVE,
} ventured_hand_gesture_t;

typedef struct {
    uint8_t last_hand;
    uint8_t pending_zone;
    bool have_hand;
    bool session;
    uint32_t last_hand_ms;
    uint32_t pending_ms;
    uint32_t lockout_until_ms;
} ventured_hand_tracker_t;

void ventured_hand_tracker_init(ventured_hand_tracker_t *tracker);
ventured_hand_gesture_t ventured_hand_tracker_update(ventured_hand_tracker_t *tracker, bool valid,
                                                     uint16_t dist_mm, uint32_t now_ms);
