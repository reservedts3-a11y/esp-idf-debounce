// Task 4: no interrupts at all. The button is read every 5 ms,
// and debounce is a state machine: a new state is accepted only after
// it was seen several times in a row.
#include "config.h"

#if TASK == 4

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

#define POLL_MS       5  // read the button every 5 ms
#define STABLE_COUNT  4  // 4 equal readings in a row = 20 ms stable

static const char *TAG = "task4";

// Four states of the button
typedef enum {
    RELEASED,        // released and stable
    MAYBE_PRESSED,   // saw "pressed", checking that it is not bounce
    PRESSED,         // pressed and stable
    MAYBE_RELEASED,  // saw "released", checking that it is not bounce
} button_state_t;

static const char *state_name[] = {"RELEASED", "MAYBE_PRESSED", "PRESSED", "MAYBE_RELEASED"};

void app_main(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << BUTTON_GPIO,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = BUTTON_PULLUP,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,  // no interrupts in this task
    };
    gpio_config(&cfg);

    ESP_LOGI(TAG, "Task 4: polling every %d ms, %d stable readings. Press the button...",
             POLL_MS, STABLE_COUNT);

    button_state_t state = RELEASED;
    int same = 0;               // how many equal readings in a row
    uint32_t press_count = 0;
    uint32_t false_starts = 0;  // MAYBE_* that turned out to be bounce
    int64_t first_seen_us = 0;  // when the press was first noticed

    TickType_t last_wake = xTaskGetTickCount();
    while (1) {
        // Exactly every POLL_MS, no drift (better than a plain vTaskDelay)
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(POLL_MS));

        bool is_down = gpio_get_level(BUTTON_GPIO) == 0;  // 0 = pressed

        switch (state) {
        case RELEASED:
            if (is_down) {
                state = MAYBE_PRESSED;
                same = 1;
                first_seen_us = esp_timer_get_time();
            }
            break;

        case MAYBE_PRESSED:
            if (!is_down) {          // jumped back -> it was bounce
                state = RELEASED;
                false_starts++;
            } else if (++same >= STABLE_COUNT) {
                state = PRESSED;     // stable long enough -> real press
                press_count++;
                ESP_LOGI(TAG, "PRESS #%lu | bounce filtered: %lu | latency: %lld us",
                         press_count, false_starts, esp_timer_get_time() - first_seen_us);
            }
            break;

        case PRESSED:
            if (!is_down) {
                state = MAYBE_RELEASED;
                same = 1;
            }
            break;

        case MAYBE_RELEASED:
            if (is_down) {           // jumped back -> it was bounce
                state = PRESSED;
                false_starts++;
            } else if (++same >= STABLE_COUNT) {
                state = RELEASED;    // released for real -> no action
                ESP_LOGI(TAG, "release (no action) | state: %s", state_name[state]);
            }
            break;
        }
    }
}

#endif
