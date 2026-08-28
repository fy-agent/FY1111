#pragma once

#include <stddef.h>
#include <stdint.h>

#include "usb/usb_ids.h"

size_t ventured_hid_pack(uint8_t report[VENTURED_HID_REPORT_LEN], const uint8_t *src, size_t len);
size_t ventured_hid_unpack(const uint8_t *report, size_t report_len, uint8_t *dst, size_t dst_len);
