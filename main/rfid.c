#include <stdio.h>
#include "leds_utils.c"

#define SPI_MASTER_HOST SPI3_HOST

#define PIN_SPI_CS 5
#define PIN_SPI_CLK 18
#define PIN_SPI_MOSI 23
#define PIN_SPI_MISO 19

#define red_on() turn_on_led(2)
#define red_off() turn_off_led(2)
#define green_on() turn_on_led(4)
#define green_off() turn_off_led(4)

void turn_on_led(uint8_t);
void turn_off_led(uint8_t);

TaskHandle_t green_led_task_handle = NULL;
TaskHandle_t red_led_task_handle = NULL;

void app_main(void)
{
    uint8_t led_flag = 0;
    while (1)
    {
        if (led_flag == 0) {
            green_off();
            vTaskDelay(100 / portTICK_PERIOD_MS);  // Delay to ensure previous LED is turned off
            red_on();
        } else {
            red_off();
            vTaskDelay(100 / portTICK_PERIOD_MS);  // Delay to ensure previous LED is turned off
            green_on();
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
        led_flag = !led_flag;
    }
    
}

void turn_on_led(uint8_t led_gpio_pin) {
    TaskHandle_t blink_led_task_handle = NULL;

    xTaskCreate(blink_led_task, "led_task", 2048, (void *)led_gpio_pin, 5, &blink_led_task_handle);
    configASSERT(blink_led_task_handle);

    if (led_gpio_pin == 2)
        red_led_task_handle = blink_led_task_handle;
    
    if (led_gpio_pin == 4)
        green_led_task_handle = blink_led_task_handle;
}

void turn_off_led(uint8_t led_gpio_pin) {
    if (led_gpio_pin == 2 && red_led_task_handle != NULL)
        vTaskDelete(red_led_task_handle);
    
    if  (led_gpio_pin == 4 && green_led_task_handle != NULL)
        vTaskDelete(green_led_task_handle);
}