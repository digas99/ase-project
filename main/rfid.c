#include <stdio.h>
#include "leds_utils.c"

#define SPI_MASTER_HOST SPI3_HOST

#define PIN_SPI_CS 5
#define PIN_SPI_CLK 18
#define PIN_SPI_MOSI 23
#define PIN_SPI_MISO 19

void app_main(void)
{
    // test led
    uint16_t freq = 100;

    TaskHandle_t blink_led_task_handle = NULL;
    xTaskCreate(blink_led_task, "blink_led_task", 2048, (void*)freq, 5, &blink_led_task_handle);
    configASSERT(blink_led_task_handle);
}
