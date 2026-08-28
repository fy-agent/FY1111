/*
 * Descriptors derived from TinyUSB hid_generic_inout + webusb_serial
 * (MIT, Copyright (c) 2019 Ha Thach / tinyusb.org).
 */

#include "class/hid/hid_device.h"
#include "tusb.h"
#include "usb_ids.h"

#define VENDOR_REQUEST_MICROSOFT 1
#define MS_OS_20_DESC_LEN 0xB2

enum {
    ITF_NUM_HID = 0,
    ITF_NUM_VENDOR,
    ITF_NUM_TOTAL
};

#define EPNUM_HID_OUT 0x01
#define EPNUM_HID_IN 0x81
#define EPNUM_VENDOR_OUT 0x02
#define EPNUM_VENDOR_IN 0x82

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_INOUT_DESC_LEN + TUD_VENDOR_DESC_LEN)

static const tusb_desc_device_t s_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0210,
    .bDeviceClass = 0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = 64,
    .idVendor = VENTURED_USB_VID,
    .idProduct = VENTURED_USB_PID,
    .bcdDevice = 0x0100,
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,
    .bNumConfigurations = 0x01
};

static const uint8_t s_hid_report[] = {
    TUD_HID_REPORT_DESC_GENERIC_INOUT(VENTURED_HID_REPORT_LEN)
};

static const uint8_t s_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),
    TUD_HID_INOUT_DESCRIPTOR(ITF_NUM_HID, 4, HID_ITF_PROTOCOL_NONE, sizeof(s_hid_report),
                             EPNUM_HID_OUT, EPNUM_HID_IN, VENTURED_HID_REPORT_LEN, 5),
    TUD_VENDOR_DESCRIPTOR(ITF_NUM_VENDOR, 5, EPNUM_VENDOR_OUT, EPNUM_VENDOR_IN, 64),
};

#define BOS_TOTAL_LEN (TUD_BOS_DESC_LEN + TUD_BOS_MICROSOFT_OS_DESC_LEN)

static const uint8_t s_bos[] = {
    TUD_BOS_DESCRIPTOR(BOS_TOTAL_LEN, 1),
    TUD_BOS_MS_OS_20_DESCRIPTOR(MS_OS_20_DESC_LEN, VENDOR_REQUEST_MICROSOFT)
};

static const uint8_t s_ms_os_20[] = {
    U16_TO_U8S_LE(0x000A), U16_TO_U8S_LE(MS_OS_20_SET_HEADER_DESCRIPTOR), U32_TO_U8S_LE(0x06030000),
    U16_TO_U8S_LE(MS_OS_20_DESC_LEN),
    U16_TO_U8S_LE(0x0008), U16_TO_U8S_LE(MS_OS_20_SUBSET_HEADER_CONFIGURATION), 0, 0,
    U16_TO_U8S_LE(MS_OS_20_DESC_LEN - 0x0A),
    U16_TO_U8S_LE(0x0008), U16_TO_U8S_LE(MS_OS_20_SUBSET_HEADER_FUNCTION), ITF_NUM_VENDOR, 0,
    U16_TO_U8S_LE(MS_OS_20_DESC_LEN - 0x0A - 0x08),
    U16_TO_U8S_LE(0x0014), U16_TO_U8S_LE(MS_OS_20_FEATURE_COMPATBLE_ID),
    'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    U16_TO_U8S_LE(MS_OS_20_DESC_LEN - 0x0A - 0x08 - 0x08 - 0x14),
    U16_TO_U8S_LE(MS_OS_20_FEATURE_REG_PROPERTY),
    U16_TO_U8S_LE(0x0007), U16_TO_U8S_LE(0x002A),
    'D', 0x00, 'e', 0x00, 'v', 0x00, 'i', 0x00, 'c', 0x00, 'e', 0x00, 'I', 0x00, 'n', 0x00,
    't', 0x00, 'e', 0x00, 'r', 0x00, 'f', 0x00, 'a', 0x00, 'c', 0x00, 'e', 0x00, 'G', 0x00,
    'U', 0x00, 'I', 0x00, 'D', 0x00, 's', 0x00, 0x00, 0x00,
    U16_TO_U8S_LE(0x0050),
    '{', 0x00, 'A', 0x00, '7', 0x00, 'E', 0x00, '3', 0x00, 'C', 0x00, '2', 0x00, 'B', 0x00,
    '1', 0x00, '-', 0x00, '4', 0x00, 'D', 0x00, '5', 0x00, 'E', 0x00, '-', 0x00, '4', 0x00,
    'F', 0x00, '8', 0x00, '0', 0x00, '-', 0x00, '9', 0x00, 'C', 0x00, '1', 0x00, 'A', 0x00,
    '-', 0x00, '2', 0x00, 'B', 0x00, '3', 0x00, 'D', 0x00, '4', 0x00, 'E', 0x00, '5', 0x00,
    'F', 0x00, '6', 0x00, '0', 0x00, '7', 0x00, '1', 0x00, '}', 0x00, 0x00, 0x00, 0x00, 0x00
};

static const char *s_strings[] = {
    (const char[]){0x09, 0x04},
    "VentureD",
    "VentureD Board C",
    "ventured-board-c",
    "VentureD HID",
    "VentureD Vendor",
};

const tusb_desc_device_t *ventured_usb_device_desc(void) {
    return &s_device;
}

const uint8_t *ventured_usb_config_desc(void) {
    return s_configuration;
}

const char **ventured_usb_string_desc(void) {
    return (const char **)s_strings;
}

size_t ventured_usb_string_count(void) {
    return sizeof(s_strings) / sizeof(s_strings[0]);
}

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
    (void)instance;
    return s_hid_report;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                               uint8_t *buffer, uint16_t reqlen) {
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;
    return 0;
}

uint8_t const *tud_descriptor_bos_cb(void) {
    return s_bos;
}

bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) {
    if (stage != CONTROL_STAGE_SETUP) return true;
    if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_VENDOR &&
        request->bRequest == VENDOR_REQUEST_MICROSOFT && request->wIndex == 7) {
        uint16_t len = request->wLength;
        if (len > MS_OS_20_DESC_LEN) len = MS_OS_20_DESC_LEN;
        return tud_control_xfer(rhport, request, (void *)s_ms_os_20, len);
    }
    return false;
}
