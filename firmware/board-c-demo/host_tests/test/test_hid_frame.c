#include "unity.h"

#include <string.h>

#include "hid_frame.h"
#include "usb/usb_ids.h"

void test_hid_pack_prefix_length_and_caps_payload(void) {
    uint8_t report[VENTURED_HID_REPORT_LEN];
    uint8_t src[80];
    memset(src, 'A', sizeof(src));
    size_t used = ventured_hid_pack(report, src, sizeof(src));
    TEST_ASSERT_EQUAL_UINT(VENTURED_HID_PAYLOAD_MAX, used);
    TEST_ASSERT_EQUAL_UINT8(VENTURED_HID_PAYLOAD_MAX, report[0]);
    TEST_ASSERT_EQUAL_INT(0, memcmp(report + 1, src, used));
}

void test_hid_unpack_accepts_raw_and_report_id_prefix(void) {
    uint8_t report[VENTURED_HID_REPORT_LEN];
    const uint8_t src[] = "VKEY_INPUT/1 {\"seq\":1}\n";
    size_t used = ventured_hid_pack(report, src, sizeof(src) - 1);
    uint8_t out[VENTURED_HID_PAYLOAD_MAX];
    TEST_ASSERT_EQUAL_UINT(used, ventured_hid_unpack(report, sizeof(report), out, sizeof(out)));
    TEST_ASSERT_EQUAL_INT(0, memcmp(out, src, used));

    uint8_t prefixed[1 + VENTURED_HID_REPORT_LEN];
    prefixed[0] = 0;
    memcpy(prefixed + 1, report, sizeof(report));
    memset(out, 0, sizeof(out));
    TEST_ASSERT_EQUAL_UINT(used, ventured_hid_unpack(prefixed, sizeof(prefixed), out, sizeof(out)));
    TEST_ASSERT_EQUAL_INT(0, memcmp(out, src, used));
}

void test_hid_unpack_rejects_empty_and_overlong_length(void) {
    uint8_t out[8];
    uint8_t empty[VENTURED_HID_REPORT_LEN] = {0};
    TEST_ASSERT_EQUAL_UINT(0, ventured_hid_unpack(empty, sizeof(empty), out, sizeof(out)));
    uint8_t overlong[4] = {4, 'a', 'b', 'c'};
    TEST_ASSERT_EQUAL_UINT(0, ventured_hid_unpack(overlong, sizeof(overlong), out, sizeof(out)));
    TEST_ASSERT_EQUAL_UINT(0, ventured_hid_pack(empty, NULL, 4));
}
