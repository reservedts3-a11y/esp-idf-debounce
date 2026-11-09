// Task 1: basic interrupt on FALLING edge, no debounce.
// Shows the problem: one physical press gives several interrupts.
#include "config.h"

#if TASK == 1

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "task1";

// Changed inside the interrupt -> must be volatile
static volatile uint32_t isr_count = 0;

// Interrupt handler: ONLY increments the counter.
// IRAM_ATTR keeps it in fast RAM; no printing here - ISR must be very short
static void IRAM_ATTR button_isr(void *arg)
{
    isr_count++;
}

void app_main(void)
{
    // Button pin: input, internal pull-up (released = 1, pressed = 0),
    // interrupt on FALLING edge (1 -> 0)
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << BUTTON_GPIO,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = BUTTON_PULLUP,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&cfg);

    gpio_install_isr_service(0);  // enable GPIO interrupts
    gpio_isr_handler_add(BUTTON_GPIO, button_isr, NULL);

    ESP_LOGI(TAG, "Task 1: no debounce. Press the button...");

    uint32_t last = 0;
    while (1) {
        uint32_t now = isr_count;
        if (now != last) {
            // "+N" - how many interrupts came since the last print
            ESP_LOGI(TAG, "interrupts: %lu (+%lu)", now, now - last);
            last = now;
        }
        vTaskDelay(pdMS_TO_TICKS(10));  // check the counter every 10 ms
    }
}

#endif
