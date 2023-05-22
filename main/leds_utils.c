#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "led_strip.h"
#include "sdkconfig.h"
#include "esp_log.h"

void blink_led_task(void*);

static void blink_led(uint8_t led_gpio, uint8_t s_led_state)
{
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(led_gpio, s_led_state);
}

static void configure_led(uint8_t led_gpio)
{
    gpio_reset_pin(led_gpio);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(led_gpio, GPIO_MODE_OUTPUT);
}

// portTICK_PERIOD_MS is the temporal resolution of the FreeRTOS tick timer

void blink_led_task(void* gpio_pin) {
    uint8_t pin = (uint8_t)gpio_pin; 

    configure_led(pin);

    uint32_t freq = 100;
    uint32_t period = 1000 / freq; // period in ms

    uint8_t s_led_state = 0;
    while (1) {
        blink_led(pin, s_led_state);
        s_led_state = !s_led_state;
        vTaskDelay(period / portTICK_PERIOD_MS);
    }
}