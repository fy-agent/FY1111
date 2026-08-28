#include "unity.h"

#include "tof_hand_gesture.h"

static void confirm(ventured_hand_tracker_t *tracker, uint16_t mm, uint32_t *now, ventured_hand_gesture_t expect) {
    TEST_ASSERT_EQUAL_INT(VENTURED_HAND_NONE, ventured_hand_tracker_update(tracker, true, mm, *now));
    *now += VENTURED_HAND_DWELL_MS;
    TEST_ASSERT_EQUAL_INT(expect, ventured_hand_tracker_update(tracker, true, mm, *now));
}

void test_far_to_near_starts_and_near_to_far_stops(void) {
    ventured_hand_tracker_t tracker;
    ventured_hand_tracker_init(&tracker);
    uint32_t now = 0;
    TEST_ASSERT_EQUAL_INT(VENTURED_HAND_NONE, ventured_hand_tracker_update(&tracker, true, 40, now));
    now = 50;
    confirm(&tracker, 220, &now, VENTURED_HAND_NONE);
    now += 80;
    confirm(&tracker, 100, &now, VENTURED_HAND_ENTER);
    now += VENTURED_HAND_LOCKOUT_MS;
    confirm(&tracker, 210, &now, VENTURED_HAND_LEAVE);
}

void test_sky_band_is_not_a_near_hand(void) {
    ventured_hand_tracker_t tracker;
    ventured_hand_tracker_init(&tracker);
    uint32_t now = 0;
    confirm(&tracker, 220, &now, VENTURED_HAND_NONE);
    TEST_ASSERT_EQUAL_INT(VENTURED_HAND_NONE, ventured_hand_tracker_update(&tracker, true, 50, now + 40));
    now += 80;
    confirm(&tracker, 110, &now, VENTURED_HAND_ENTER);
}

void test_holding_near_then_vanishing_to_sky_stops(void) {
    ventured_hand_tracker_t tracker;
    ventured_hand_tracker_init(&tracker);
    uint32_t now = 0;
    confirm(&tracker, 200, &now, VENTURED_HAND_NONE);
    now += 80;
    confirm(&tracker, 90, &now, VENTURED_HAND_ENTER);
    TEST_ASSERT_EQUAL_INT(VENTURED_HAND_NONE, ventured_hand_tracker_update(&tracker, true, 40, now + 200));
    TEST_ASSERT_EQUAL_INT(VENTURED_HAND_LEAVE,
                          ventured_hand_tracker_update(&tracker, true, 40, now + VENTURED_HAND_MEMORY_MS + 1));
}

void test_withdraw_bounce_during_lockout_does_not_reenter(void) {
    ventured_hand_tracker_t tracker;
    ventured_hand_tracker_init(&tracker);
    uint32_t now = 0;
    confirm(&tracker, 220, &now, VENTURED_HAND_NONE);
    now += 80;
    confirm(&tracker, 100, &now, VENTURED_HAND_ENTER);
    now += VENTURED_HAND_LOCKOUT_MS;
    confirm(&tracker, 210, &now, VENTURED_HAND_LEAVE);
    TEST_ASSERT_EQUAL_INT(VENTURED_HAND_NONE, ventured_hand_tracker_update(&tracker, true, 100, now + 20));
    TEST_ASSERT_EQUAL_INT(VENTURED_HAND_NONE,
                          ventured_hand_tracker_update(&tracker, true, 100, now + 20 + VENTURED_HAND_DWELL_MS));
}

void test_holding_near_does_not_timeout(void) {
    ventured_hand_tracker_t tracker;
    ventured_hand_tracker_init(&tracker);
    uint32_t now = 0;
    confirm(&tracker, 200, &now, VENTURED_HAND_NONE);
    now += 80;
    confirm(&tracker, 90, &now, VENTURED_HAND_ENTER);
    TEST_ASSERT_EQUAL_INT(VENTURED_HAND_NONE,
                          ventured_hand_tracker_update(&tracker, true, 95, now + VENTURED_HAND_MEMORY_MS + 100));
}

void test_single_sample_is_not_a_gesture(void) {
    ventured_hand_tracker_t tracker;
    ventured_hand_tracker_init(&tracker);
    TEST_ASSERT_EQUAL_INT(VENTURED_HAND_NONE, ventured_hand_tracker_update(&tracker, true, 220, 0));
    TEST_ASSERT_EQUAL_INT(VENTURED_HAND_NONE, ventured_hand_tracker_update(&tracker, true, 100, 40));
}
