// Task 3: debounce by checking the pin level.
// The interrupt only says "something happened". The task waits until the
// contacts calm down and accepts the press only if the button is STILL pressed.
// Releasing the button never counts.
#include "config.h"

#if TASK == 3

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

#define SETTLE_MS 30  // how long to wait for the bounce to end

static const char *TAG = "task3";

static TaskHandle_t button_task;           // who gets the "event" signal
static volatile uint32_t raw_count = 0;    // all interrupts (with bounce)
static volatile int64_t first_edge_us = 0; // time of the first edge of an event

// ISR: counts the edge and pokes the task. Nothing else.
static void IRAM_ATTR button_isr(void *arg)
{
    raw_count++;
    if (first_edge_us == 0) {
        first_edge_us = esp_timer_get_time();
    }
    BaseType_t woken = pdFALSE;
    vTaskNotifyGiveFromISR(button_task, &woken);  // "event!" signal
    portYIELD_FROM_ISR(woken);
}

static void button_task_fn(void *arg)
{
    bool pressed = false;  // state already accepted: is the button held now?
    uint32_t accepted_count = 0;

    while (1) {
        // 1. Sleep until the ISR says "event"
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // 2. Wait until the contacts stop bouncing
        vTaskDelay(pdMS_TO_TICKS(SETTLE_MS));
        ulTaskNotifyTake(pdTRUE, 0);  // drop the bounce signals that came meanwhile

        // 3. Look at the real level now: 0 = pressed, 1 = released
        int level = gpio_get_level(BUTTON_GPIO);
        int64_t edge_us = first_edge_us;
        first_edge_us = 0;

        if (level == 0 && !pressed) {
            // Still pressed and was not pressed before -> real press
            pressed = true;
            accepted_count++;
            ESP_LOGI(TAG, "PRESS #%lu | raw interrupts: %lu | latency: %lld us",
                     accepted_count, raw_count, esp_timer_get_time() - edge_us);
        } else if (level == 1 && pressed) {
            // Button released -> no action, just remember the state
            pressed = false;
            ESP_LOGI(TAG, "release ignored | raw interrupts: %lu", raw_count);
        }
    }
}

void app_main(void)
{
    xTaskCreate(button_task_fn, "button", 4096, NULL, 10, &button_task);

    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << BUTTON_GPIO,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = BUTTON_PULLUP,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        // ANY edge: we must also notice the release, otherwise "pressed"
        // would stay true forever after a clean release
        .intr_type    = GPIO_INTR_ANYEDGE,
    };
    gpio_config(&cfg);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO, button_isr, NULL);

    ESP_LOGI(TAG, "Task 3: state-based debounce (settle %d ms). Press the button...",
             SETTLE_MS);
}

#endif
