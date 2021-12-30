#include <stdint.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "tusb_cdc_acm.h"
#include "tusb_tasks.h"
#include "sdkconfig.h"
#include "sched.h" // sched_main
#include "command.h" // DECL_CONSTANT
#include "esp/pgm.h" // READP

static const char *TAG = "usb-cdc";
static uint32_t bitrate;
static int dtr;

static uint8_t transmit_buf[192];
static uint8_t receive_buf[128], receive_pos;

void console_sendf(const struct command_encoder *ce, va_list args)
{
    // Verify space for message
    uint_fast8_t max_size = READP(ce->max_size);
    if (max_size > tud_cdc_write_available())
        // Not enough space for message
        return;

    // Generate message
    uint8_t *buf = &transmit_buf[0];
    uint_fast8_t msglen = command_encode_and_frame(buf, ce, args);
    ESP_LOGI(TAG, "Sending (%d bytes)", msglen);
    tud_cdc_write(transmit_buf,  msglen);
}

static void tinyusb_cdc_rx_callback(int itf, cdcacm_event_t *event)
{
    /* initialization */
    size_t rx_size = 0;
    uint_fast8_t pop_count;

    /* read */
    esp_err_t ret = tinyusb_cdcacm_read(itf, receive_buf, sizeof(receive_buf) - receive_pos, &rx_size);
    if (ret == ESP_OK) {
        receive_pos += rx_size;
        // Process a message block
        do {
            int_fast8_t ret = command_find_and_dispatch(receive_buf, receive_pos, &pop_count);
            if (ret) {
                ESP_LOGI(TAG, "Processed (%d bytes) from %d", pop_count, receive_pos);
                // Move buffer
                uint_fast8_t needcopy = receive_pos - pop_count;
                if (needcopy) {
                    memmove(receive_buf, &receive_buf[pop_count], needcopy);
                }
                receive_pos = needcopy;
            }
        } while (ret > 0);
    } else {
        ESP_LOGE(TAG, "Read error");
    }
}

static void usb_request_bootloader(void) {
    ESP_LOGI(TAG, "Request reboot to bootloader");
}

static void check_reboot(void)
{
    if (bitrate == 1200 && !dtr) {
        // A baud of 1200 is an Arduino style request to enter the bootloader
        usb_request_bootloader();
    }
}

static void tinyusb_cdc_line_state_changed_callback(int itf, cdcacm_event_t *event)
{
    dtr = event->line_state_changed_data.dtr;
    int rst = event->line_state_changed_data.rts;
    ESP_LOGI(TAG, "Line state changed! dtr:%d, rst:%d", dtr, rst);
    check_reboot();
}

static void tinyusb_cdc_line_coding_changed_callback(int itf, cdcacm_event_t *event)
{
    bitrate = event->line_coding_changed_data.p_line_coding->bit_rate;
    ESP_LOGI(TAG, "Line coding changed! bitrate:%d", bitrate);
    check_reboot();
}


void usb_cdc_init(void)
{
    ESP_LOGI(TAG, "USB initialization");
    tinyusb_config_t tusb_cfg = {}; // the configuration using default values
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
    bitrate = 0;
    receive_pos = 0;

    tinyusb_config_cdcacm_t amc_cfg = {
        .usb_dev = TINYUSB_USBDEV_0,
        .cdc_port = TINYUSB_CDC_ACM_0,
        .rx_unread_buf_sz = 64,
        .callback_rx = &tinyusb_cdc_rx_callback,
        .callback_rx_wanted_char = NULL,
        .callback_line_state_changed = tinyusb_cdc_line_state_changed_callback,
        .callback_line_coding_changed = tinyusb_cdc_line_coding_changed_callback
    };

    ESP_ERROR_CHECK(tusb_cdc_acm_init(&amc_cfg));
    ESP_LOGI(TAG, "USB initialization DONE");
}
DECL_INIT(usb_cdc_init);

// Periodic task to process the TinyUSB work.
DECL_TASK(tud_task);
