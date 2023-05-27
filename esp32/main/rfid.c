
#include <stdio.h>
#include "leds_utils.c"
#include <esp_log.h>
#include <inttypes.h>
#include "rc522.h"

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

static const char* TAG = "rc522-demo";
static rc522_handle_t scanner;

static void rc522_handler(void* arg, esp_event_base_t base, int32_t event_id, void* event_data)
{
    rc522_event_data_t* data = (rc522_event_data_t*) event_data;

    switch(event_id) {
        case RC522_EVENT_TAG_SCANNED: {
                rc522_tag_t* tag = (rc522_tag_t*) data->ptr;
                ESP_LOGI(TAG, "Tag scanned (sn: %" PRIu64 ")", tag->serial_number);
                green_on();
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                green_off();
            }
            break;
    }
}

void app_main(void)
{
    rc522_config_t config = {
        .spi_host = VSPI_HOST,
        .spi_miso_io = PIN_SPI_MISO,
        .spi_mosi_io = PIN_SPI_MOSI,
        .spi_sck_io = PIN_SPI_CLK,
        .spi_ss_gpio = PIN_SPI_CS,
    };

    rc522_init(&config, &scanner);
    rc522_register_events(scanner, RC522_EVENT_ANY, rc522_handler, NULL);
    rc522_start(scanner);
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