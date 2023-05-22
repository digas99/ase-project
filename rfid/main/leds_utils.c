#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "led_strip.h"
#include "sdkconfig.h"
#include "esp_log.h"
    
// GPIO pin number for blinking LED
#define CONFIG_BLINK_GPIO 4

#define BLINK_GPIO CONFIG_BLINK_GPIO

void blink_led_task(void*);

static void blink_led(uint8_t s_led_state)
{
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(BLINK_GPIO, s_led_state);
}

static void configure_led(void)
{
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

// portTICK_PERIOD_MS is the temporal resolution of the FreeRTOS tick timer

void blink_led_task(void * pvParameter) {
    configure_led();

    uint32_t freq = (uint32_t)pvParameter;
    uint32_t period = 1000 / freq; // period in ms

    uint8_t s_led_state = 0;
    while (1) {
        blink_led(s_led_state);
        s_led_state = !s_led_state;
        vTaskDelay(period / portTICK_PERIOD_MS);
    }
}