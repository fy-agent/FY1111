#pragma once

#include <stddef.h>
#include "tusb.h"

const tusb_desc_device_t *ventured_usb_device_desc(void);
const uint8_t *ventured_usb_config_desc(void);
const char **ventured_usb_string_desc(void);
size_t ventured_usb_string_count(void);
