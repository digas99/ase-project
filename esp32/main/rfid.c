#include <stdio.h>
#include <esp_log.h>
#include <inttypes.h>

#include "rc522.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#include "leds_utils.c"

#define SPI_MASTER_HOST SPI3_HOST

#define PIN_SPI_CS 5
#define PIN_SPI_CLK 18
#define PIN_SPI_MOSI 23
#define PIN_SPI_MISO 19

#define PIN_GREEN_LED 4
#define PIN_RED_LED 2

#define PIN_BUZZER 33
#define BUZZER_CHANNEL  LEDC_CHANNEL_0
#define BUZZER_FREQ_HZ  2000
#define BUZZER_RESOLUTION LEDC_TIMER_13_BIT

#define red_on() turn_on_led(PIN_RED_LED)
#define red_off() turn_off_led(PIN_RED_LED)
#define green_on() turn_on_led(PIN_GREEN_LED)
#define green_off() turn_off_led(PIN_GREEN_LED)

void turn_on_led(uint8_t);
void turn_off_led(uint8_t);

void ledc_buzzer_init();
void buzzer(float);

bool request_access(uint64_t);
void req_seq();
void forb_seq();

TaskHandle_t green_led_task_handle = NULL;
TaskHandle_t red_led_task_handle = NULL;

static const char* TAG = "rc522";
static rc522_handle_t scanner;

static void rc522_handler(void* arg, esp_event_base_t base, int32_t event_id, void* event_data)
{
    rc522_event_data_t* data = (rc522_event_data_t*) event_data;

    switch(event_id) {
        case RC522_EVENT_TAG_SCANNED: {
                rc522_tag_t* tag = (rc522_tag_t*) data->ptr;
                ESP_LOGI(TAG, "Tag scanned (sn: %" PRIu64 ")", tag->serial_number);
                
                if (request_access(tag->serial_number)) {
                    ESP_LOGI(TAG, "Access granted");
                }
                else {
                    forb_seq();
                    ESP_LOGI(TAG, "Access denied");
                }
            }
            break;
    }
}

void app_main(void)
{
    ledc_buzzer_init();

    rc522_config_t config = {
        .spi.host = VSPI_HOST,
        .spi.miso_gpio = PIN_SPI_MISO,
        .spi.mosi_gpio = PIN_SPI_MOSI,
        .spi.sck_gpio = PIN_SPI_CLK,
        .spi.sda_gpio = PIN_SPI_CS,
    };

    rc522_create(&config, &scanner);
    rc522_register_events(scanner, RC522_EVENT_ANY, rc522_handler, NULL);
    rc522_start(scanner);
}

bool request_access(uint64_t serial_number) {
    req_seq();
    
    // simulate request delay
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    
    // clear req_sec
    green_off();
    buzzer(0);

    // simulate response
    return serial_number == 62984291464;
}

void req_seq() {
    green_on();
    buzzer(1);
}

void forb_seq() {
    red_on();
    for (int i = 0; i < 3; i++) {
        buzzer(0.80);
        
        vTaskDelay(250 / portTICK_PERIOD_MS);

        buzzer(0);

        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
    red_off();
}

void turn_on_led(uint8_t led_gpio_pin) {
    TaskHandle_t blink_led_task_handle = NULL;

    xTaskCreate(blink_led_task, "led_task", 2048, (void *)led_gpio_pin, 5, &blink_led_task_handle);
    configASSERT(blink_led_task_handle);

    if (led_gpio_pin == PIN_RED_LED)
        red_led_task_handle = blink_led_task_handle;
    
    if (led_gpio_pin == PIN_GREEN_LED)
        green_led_task_handle = blink_led_task_handle;
}

void turn_off_led(uint8_t led_gpio_pin) {
    if (led_gpio_pin == PIN_RED_LED && red_led_task_handle != NULL)
        vTaskDelete(red_led_task_handle);
    
    if  (led_gpio_pin == PIN_GREEN_LED && green_led_task_handle != NULL)
        vTaskDelete(green_led_task_handle);
}

void ledc_buzzer_init() {
    // Configure the LEDC peripheral for PWM generation
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = BUZZER_RESOLUTION,
        .freq_hz = BUZZER_FREQ_HZ,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_0
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .channel = BUZZER_CHANNEL,
        .duty = 0,
        .gpio_num = PIN_BUZZER,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_sel = LEDC_TIMER_0
    };
    ledc_channel_config(&ledc_channel);
}

void buzzer(float duty_cycle) {
    uint32_t duty = (uint32_t)((1 << BUZZER_RESOLUTION) * duty_cycle);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, BUZZER_CHANNEL, duty);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, BUZZER_CHANNEL);
}