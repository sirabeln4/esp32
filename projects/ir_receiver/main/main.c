#include <inttypes.h>
#include <stdio.h>
#include "driver/gpio.h"
#include "driver/rmt_rx.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "nec_decoder.h"

#define IR_RX_GPIO GPIO_NUM_0 // XIAO D0; ESP-IDF uses chip GPIO numbers.
#define RX_SYMBOLS 64
#define REPEAT_TIMEOUT_US 150000
static const char *TAG = "ir_receiver";

static bool receive_done(rmt_channel_handle_t channel,
                         const rmt_rx_done_event_data_t *event, void *context)
{
    (void)channel;
    BaseType_t wake = pdFALSE;
    xQueueSendFromISR((QueueHandle_t)context, event, &wake);
    return wake == pdTRUE;
}

static size_t unpack(const rmt_rx_done_event_data_t *event, ir_pulse_t *pulses)
{
    size_t count = 0;
    for (size_t i = 0; i < event->num_symbols; ++i) {
        rmt_symbol_word_t s = event->received_symbols[i];
        if (s.duration0) pulses[count++] = (ir_pulse_t){s.duration0, s.level0 == 0};
        if (s.duration1) pulses[count++] = (ir_pulse_t){s.duration1, s.level1 == 0};
    }
    return count;
}

void app_main(void)
{
    QueueHandle_t queue = xQueueCreate(1, sizeof(rmt_rx_done_event_data_t));
    ESP_RETURN_VOID_ON_FALSE(queue, TAG, "Cannot create receive queue");
    rmt_channel_handle_t channel = NULL;
    rmt_rx_channel_config_t config = {
        .gpio_num = IR_RX_GPIO,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 1000000, // One tick per microsecond.
        .mem_block_symbols = RX_SYMBOLS,
    };
    ESP_ERROR_CHECK(rmt_new_rx_channel(&config, &channel));
    ESP_ERROR_CHECK(gpio_set_pull_mode(IR_RX_GPIO, GPIO_PULLUP_ONLY));
    rmt_rx_event_callbacks_t callbacks = {.on_recv_done = receive_done};
    ESP_ERROR_CHECK(rmt_rx_register_event_callbacks(channel, &callbacks, queue));
    ESP_ERROR_CHECK(rmt_enable(channel));
    rmt_receive_config_t receive_config = {
        .signal_range_min_ns = 1250, // Hardware glitch filter (1.25 us).
        .signal_range_max_ns = 12000000, // 12 ms idle ends a frame.
    };
    static rmt_symbol_word_t symbols[RX_SYMBOLS];
    static ir_pulse_t pulses[RX_SYMBOLS * 2];
    nec_frame_t last = {0};
    int64_t last_time = 0;
    bool have_last = false;
    ESP_LOGI(TAG, "Ready: VS1838B OUT -> D0/GPIO0. NEC + extended NEC; other frames show raw timings.");
    ESP_ERROR_CHECK(rmt_receive(channel, symbols, sizeof(symbols), &receive_config));
    for (;;) {
        rmt_rx_done_event_data_t event;
        if (xQueueReceive(queue, &event, portMAX_DELAY) != pdTRUE) continue;
        // Copy before rearming: the driver owns the receive buffer.
        size_t count = unpack(&event, pulses);
        ESP_ERROR_CHECK(rmt_receive(channel, symbols, sizeof(symbols), &receive_config));
        nec_frame_t frame;
        nec_result_t result = nec_decode(pulses, count, &frame);
        int64_t now = esp_timer_get_time();
        if (result == NEC_FRAME) {
            bool repeated = have_last && last.raw == frame.raw && now - last_time < REPEAT_TIMEOUT_US;
            ESP_LOGI(TAG, "%s address=0x%04X command=0x%02X raw=0x%08" PRIX32 "%s",
                     frame.extended ? "Extended NEC" : "NEC", (unsigned)frame.address,
                     (unsigned)frame.command, frame.raw, repeated ? " (full-frame repeat)" : "");
            last = frame;
            have_last = true;
            last_time = now;
        } else if (result == NEC_REPEAT && have_last && now - last_time < REPEAT_TIMEOUT_US) {
            ESP_LOGI(TAG, "NEC repeat address=0x%04X command=0x%02X", (unsigned)last.address, (unsigned)last.command);
            last_time = now;
        } else {
            have_last = false;
            ESP_LOGI(TAG, "%s: %u pulses%s. Raw microseconds: +mark -space",
                     result == NEC_REPEAT ? "Orphan NEC repeat" : "Unknown/invalid frame",
                     (unsigned)count, event.num_symbols == RX_SYMBOLS ? " (possibly truncated)" : "");
            for (size_t i = 0; i < count; ++i) {
                printf("%c%" PRIu32 "%s", pulses[i].mark ? '+' : '-', pulses[i].duration_us,
                       (i + 1) % 16 == 0 ? "\n" : " ");
            }
            printf("\n");
        }
    }
}
