#include "ventured_link.h"

#include <string.h>

#include "class/hid/hid_device.h"
#include "class/vendor/vendor_device.h"
#include "esp_log.h"
#include "freertos/stream_buffer.h"
#include "protocol/hid_frame.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "usb_descriptors.h"

static const char *TAG = "board_c_usb";
static StreamBufferHandle_t s_rx;

static void push_rx(const uint8_t *data, size_t len) {
    if (s_rx == NULL || data == NULL || len == 0) return;
    xStreamBufferSend(s_rx, data, len, 0);
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type,
                           uint8_t const *buffer, uint16_t bufsize) {
    (void)instance;
    (void)report_id;
    (void)report_type;
    uint8_t payload[VENTURED_HID_PAYLOAD_MAX];
    size_t n = ventured_hid_unpack(buffer, bufsize, payload, sizeof(payload));
    push_rx(payload, n);
}

static void drain_vendor(void) {
    uint8_t buf[64];
    while (tud_vendor_available()) {
        uint32_t n = tud_vendor_read(buf, sizeof(buf));
        push_rx(buf, n);
    }
}

static void write_hid(const uint8_t *data, size_t len) {
    while (len > 0) {
        uint8_t report[VENTURED_HID_REPORT_LEN];
        size_t chunk = ventured_hid_pack(report, data, len);
        if (chunk == 0) return;
        for (int tries = 0; tries < 20 && !tud_hid_ready(); ++tries) {
            vTaskDelay(pdMS_TO_TICKS(2));
        }
        if (tud_hid_ready()) {
            tud_hid_report(0, report, sizeof(report));
        }
        data += chunk;
        len -= chunk;
    }
}

void ventured_link_write(const char *line) {
    if (line == NULL || line[0] == '\0') return;
    size_t len = strlen(line);
    if (tud_vendor_mounted()) {
        tud_vendor_write(line, len);
        tud_vendor_write_flush();
    }
    if (tud_mounted()) {
        write_hid((const uint8_t *)line, len);
    }
}

bool ventured_link_readline(char *line, size_t capacity, TickType_t wait) {
    if (line == NULL || capacity < 2) return false;
    drain_vendor();
    size_t used = 0;
    TickType_t left = wait;
    while (used + 1 < capacity) {
        uint8_t ch = 0;
        size_t got = xStreamBufferReceive(s_rx, &ch, 1, left);
        if (got == 0) break;
        left = 0;
        if (ch == '\r') continue;
        if (ch == '\n') {
            line[used] = '\0';
            return used > 0;
        }
        line[used++] = (char)ch;
    }
    line[used] = '\0';
    return false;
}

bool ventured_link_mounted(void) {
    return tud_mounted();
}

esp_err_t ventured_link_start(void) {
    s_rx = xStreamBufferCreate(2048, 1);
    if (s_rx == NULL) return ESP_ERR_NO_MEM;
    tinyusb_config_t cfg = TINYUSB_DEFAULT_CONFIG();
    cfg.descriptor.device = ventured_usb_device_desc();
    cfg.descriptor.full_speed_config = ventured_usb_config_desc();
    cfg.descriptor.string = ventured_usb_string_desc();
    cfg.descriptor.string_count = ventured_usb_string_count();
#if TUD_OPT_HIGH_SPEED
    cfg.descriptor.high_speed_config = ventured_usb_config_desc();
#endif
    esp_err_t err = tinyusb_driver_install(&cfg);
    if (err != ESP_OK) return err;
    ESP_LOGI(TAG, "hid+vendor link ready vid=0x%04x pid=0x%04x", VENTURED_USB_VID, VENTURED_USB_PID);
    return ESP_OK;
}
