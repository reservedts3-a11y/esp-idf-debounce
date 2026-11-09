// Task 2: software debounce by time.
// An interrupt is ignored if less than 50 ms passed since the last accepted one.
// The decision is made OUTSIDE the ISR, in a normal task.
#include "config.h"

#if TASK == 2

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

#define DEBOUNCE_US (50 * 1000)  // 50 ms

static const char *TAG = "task2";

// Queue "mailbox": ISR puts edge times here, the task takes them out
static QueueHandle_t edge_queue;

// ISR: only takes the current time and drops it into the queue
static void IRAM_ATTR button_isr(void *arg)
{
    int64_t now_us = esp_timer_get_time();
    BaseType_t woken = pdFALSE;
    xQueueSendFromISR(edge_queue, &now_us, &woken);
    portYIELD_FROM_ISR(woken);  // wake the task right away if it was waiting
}

void app_main(void)
{
    edge_queue = xQueueCreate(64, sizeof(int64_t));

    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << BUTTON_GPIO,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = BUTTON_PULLUP,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_NEGEDGE,  // FALLING: 1 -> 0
    };
    gpio_config(&cfg);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO, button_isr, NULL);

    ESP_LOGI(TAG, "Task 2: time-based debounce (%d ms). Press the button...",
             DEBOUNCE_US / 1000);

    uint32_t raw_count = 0;       // all interrupts (with bounce)
    uint32_t accepted_count = 0;  // interrupts that passed the filter
    int64_t last_accepted_us = -DEBOUNCE_US;  // so the very first edge is accepted

    while (1) {
        int64_t edge_us;
        // Sleep until the ISR sends something (no CPU is wasted while waiting)
        if (xQueueReceive(edge_queue, &edge_us, portMAX_DELAY)) {
            raw_count++;

            // The whole debounce: too soon after the last accepted -> bounce, ignore
            if (edge_us - last_accepted_us < DEBOUNCE_US) {
                continue;
            }
            last_accepted_us = edge_us;
            accepted_count++;

            // Latency: from the edge to the moment the task handled it
            int64_t latency_us = esp_timer_get_time() - edge_us;
            ESP_LOGI(TAG, "ACCEPTED #%lu | raw interrupts: %lu | latency: %lld us",
                     accepted_count, raw_count, latency_us);
        }
    }
}

#endif
