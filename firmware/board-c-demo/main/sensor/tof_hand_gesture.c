#include "tof_hand_gesture.h"

#include <stddef.h>

#define SKY_MAX_MM 74U
#define NEAR_MAX_MM 135U
#define FAR_MIN_MM 180U

enum {
    ZONE_EMPTY = 0,
    ZONE_NEAR = 1,
    ZONE_FAR = 2,
    ZONE_MID = 3,
};

void ventured_hand_tracker_init(ventured_hand_tracker_t *tracker) {
    if (tracker == NULL) return;
    tracker->last_hand = ZONE_EMPTY;
    tracker->pending_zone = ZONE_EMPTY;
    tracker->have_hand = false;
    tracker->session = false;
    tracker->last_hand_ms = 0;
    tracker->pending_ms = 0;
    tracker->lockout_until_ms = 0;
}

static uint8_t classify_zone(bool valid, uint16_t dist_mm) {
    if (!valid || dist_mm <= SKY_MAX_MM) return ZONE_EMPTY;
    if (dist_mm <= NEAR_MAX_MM) return ZONE_NEAR;
    if (dist_mm >= FAR_MIN_MM) return ZONE_FAR;
    return ZONE_MID;
}

ventured_hand_gesture_t ventured_hand_tracker_update(ventured_hand_tracker_t *tracker, bool valid,
                                                     uint16_t dist_mm, uint32_t now_ms) {
    if (tracker == NULL) return VENTURED_HAND_NONE;
    uint8_t zone = classify_zone(valid, dist_mm);
    if (tracker->session && (zone == ZONE_NEAR || zone == ZONE_MID)) {
        tracker->last_hand_ms = now_ms;
    }

    if (tracker->session && (now_ms - tracker->last_hand_ms) > VENTURED_HAND_MEMORY_MS) {
        tracker->session = false;
        tracker->have_hand = false;
        tracker->lockout_until_ms = now_ms + VENTURED_HAND_LOCKOUT_MS;
        return VENTURED_HAND_LEAVE;
    }
    if (tracker->have_hand && !tracker->session &&
        (now_ms - tracker->last_hand_ms) > VENTURED_HAND_MEMORY_MS) {
        tracker->have_hand = false;
        tracker->last_hand = ZONE_EMPTY;
    }
    if (zone != ZONE_NEAR && zone != ZONE_FAR) return VENTURED_HAND_NONE;
    if (now_ms < tracker->lockout_until_ms) return VENTURED_HAND_NONE;

    if (zone != tracker->pending_zone) {
        tracker->pending_zone = zone;
        tracker->pending_ms = now_ms;
        return VENTURED_HAND_NONE;
    }
    if ((now_ms - tracker->pending_ms) < VENTURED_HAND_DWELL_MS) return VENTURED_HAND_NONE;

    ventured_hand_gesture_t gesture = VENTURED_HAND_NONE;
    if (tracker->have_hand) {
        if (!tracker->session && tracker->last_hand == ZONE_FAR && zone == ZONE_NEAR) {
            gesture = VENTURED_HAND_ENTER;
            tracker->session = true;
        } else if (tracker->session && tracker->last_hand == ZONE_NEAR && zone == ZONE_FAR) {
            gesture = VENTURED_HAND_LEAVE;
            tracker->session = false;
        }
    }
    tracker->last_hand = zone;
    tracker->have_hand = true;
    tracker->last_hand_ms = now_ms;
    if (gesture != VENTURED_HAND_NONE) {
        tracker->lockout_until_ms = now_ms + VENTURED_HAND_LOCKOUT_MS;
    }
    return gesture;
}
