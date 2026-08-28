#include "unity.h"

#include "rec_gate.h"

void test_rising_edge_starts_and_release_waits_min_then_stops(void) {
    ventured_rec_gate_t gate;
    ventured_rec_gate_init(&gate);
    TEST_ASSERT_EQUAL_INT(VENTURED_REC_GATE_START, ventured_rec_gate_update(&gate, true, false, false, 0));
    TEST_ASSERT_EQUAL_INT(VENTURED_REC_GATE_NONE, ventured_rec_gate_update(&gate, false, false, false, 100));
    TEST_ASSERT_EQUAL_INT(VENTURED_REC_GATE_STOP,
                          ventured_rec_gate_update(&gate, false, false, false, VENTURED_REC_MIN_MS));
}

void test_asr_busy_rising_edge_cancels_once_and_needs_release(void) {
    ventured_rec_gate_t gate;
    ventured_rec_gate_init(&gate);
    TEST_ASSERT_EQUAL_INT(VENTURED_REC_GATE_CANCEL_ASR,
                          ventured_rec_gate_update(&gate, true, true, false, 10));
    TEST_ASSERT_EQUAL_INT(VENTURED_REC_GATE_NONE, ventured_rec_gate_update(&gate, true, true, false, 20));
    TEST_ASSERT_EQUAL_INT(VENTURED_REC_GATE_NONE, ventured_rec_gate_update(&gate, true, false, false, 30));
    TEST_ASSERT_EQUAL_INT(VENTURED_REC_GATE_NONE, ventured_rec_gate_update(&gate, false, false, false, 40));
    TEST_ASSERT_EQUAL_INT(VENTURED_REC_GATE_START, ventured_rec_gate_update(&gate, true, false, false, 50));
}

void test_cooldown_and_held_level_do_not_restart(void) {
    ventured_rec_gate_t gate;
    ventured_rec_gate_init(&gate);
    TEST_ASSERT_EQUAL_INT(VENTURED_REC_GATE_START, ventured_rec_gate_update(&gate, true, false, false, 0));
    TEST_ASSERT_EQUAL_INT(VENTURED_REC_GATE_STOP,
                          ventured_rec_gate_update(&gate, false, false, false, VENTURED_REC_MIN_MS));
    TEST_ASSERT_EQUAL_INT(VENTURED_REC_GATE_NONE, ventured_rec_gate_update(&gate, true, false, false, VENTURED_REC_MIN_MS + 10));
    TEST_ASSERT_EQUAL_INT(VENTURED_REC_GATE_NONE,
                          ventured_rec_gate_update(&gate, true, false, false, VENTURED_REC_MIN_MS + VENTURED_REC_COOLDOWN_MS + 10));
    TEST_ASSERT_EQUAL_INT(VENTURED_REC_GATE_NONE,
                          ventured_rec_gate_update(&gate, false, false, false, VENTURED_REC_MIN_MS + VENTURED_REC_COOLDOWN_MS + 20));
    TEST_ASSERT_EQUAL_INT(
        VENTURED_REC_GATE_START,
        ventured_rec_gate_update(&gate, true, false, false, VENTURED_REC_MIN_MS + VENTURED_REC_COOLDOWN_MS + 30));
}

void test_force_stop_holds_off_until_release(void) {
    ventured_rec_gate_t gate;
    ventured_rec_gate_init(&gate);
    TEST_ASSERT_EQUAL_INT(VENTURED_REC_GATE_START, ventured_rec_gate_update(&gate, true, false, false, 0));
    TEST_ASSERT_EQUAL_INT(VENTURED_REC_GATE_STOP, ventured_rec_gate_update(&gate, true, false, true, 50));
    TEST_ASSERT_EQUAL_INT(VENTURED_REC_GATE_NONE, ventured_rec_gate_update(&gate, true, false, false, 80));
}
