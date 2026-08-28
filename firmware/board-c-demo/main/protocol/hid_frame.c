#include "hid_frame.h"

#include <string.h>

size_t ventured_hid_pack(uint8_t report[VENTURED_HID_REPORT_LEN], const uint8_t *src, size_t len) {
    if (report == NULL || src == NULL || len == 0) return 0;
    size_t chunk = len > VENTURED_HID_PAYLOAD_MAX ? VENTURED_HID_PAYLOAD_MAX : len;
    memset(report, 0, VENTURED_HID_REPORT_LEN);
    report[0] = (uint8_t)chunk;
    memcpy(report + 1, src, chunk);
    return chunk;
}

size_t ventured_hid_unpack(const uint8_t *report, size_t report_len, uint8_t *dst, size_t dst_len) {
    if (report == NULL || dst == NULL || report_len == 0 || dst_len == 0) return 0;
    size_t offset = 0;
    if (report_len >= 2 && report[0] == 0) {
        offset = 1;
    }
    if (offset >= report_len) return 0;
    size_t payload = report[offset];
    if (payload == 0 || payload > VENTURED_HID_PAYLOAD_MAX) return 0;
    if (offset + 1 + payload > report_len) return 0;
    if (payload > dst_len) payload = dst_len;
    memcpy(dst, report + offset + 1, payload);
    return payload;
}
