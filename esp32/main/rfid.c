#include <stdio.h>
#include <esp_log.h>
#include <inttypes.h>

#include "rc522.h"
#include "spi_25LC040A_eeprom.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#include "leds_utils.c"

#define PIN_SPI_CLK 18
#define PIN_SPI_MOSI 23
#define PIN_SPI_MISO 19
#define PIN_RC55_CS 5
#define PIN_EEPROM_CS 16
#define CLK_SPEED_HZ 1000000

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

esp_err_t rc522_init(bool);

void turn_on_led(uint8_t);
void turn_off_led(uint8_t);

void ledc_buzzer_init();
void buzzer(float);

bool request_access(uint64_t);
void req_seq();
void forb_seq();

TaskHandle_t green_led_task_handle = NULL;
TaskHandle_t red_led_task_handle = NULL;

volatile bool write_test = true;

static const char* RC522_TAG = "rc522";
static rc522_handle_t scanner;

static const char* EEPROM_TAG = "eeprom";
spi_device_handle_t spi_device;

static void rc522_handler(void* arg, esp_event_base_t base, int32_t event_id, void* event_data)
{
    rc522_event_data_t* data = (rc522_event_data_t*) event_data;

    switch(event_id) {
        case RC522_EVENT_TAG_SCANNED: {
                rc522_tag_t* tag = (rc522_tag_t*) data->ptr;
                ESP_LOGI(RC522_TAG, "Tag scanned (sn: %" PRIu64 ")", tag->serial_number);
                
                if (write_test) {
                    uint8_t data[1] = {0x05};
                    spi_25LC040_write_enable(spi_device);
                    spi_25LC040_write_byte(spi_device, 0x00, data[0]);
                    spi_25LC040_write_disable(spi_device);
                    ESP_LOGI(EEPROM_TAG, "Write data: 0x%02X", data[0]);
                }
                else {
                    uint8_t data[1];
                    spi_25LC040_read_byte(spi_device, 0x00, data);
                    ESP_LOGI(EEPROM_TAG, "Read data: 0x%02X", data[0]);
                }
                write_test = !write_test;
                
                if (request_access(tag->serial_number)) {
                    ESP_LOGI(RC522_TAG, "Access granted");
                }
                else {
                    forb_seq();
                    ESP_LOGI(RC522_TAG, "Access denied");
                    }
            }
            break;
    }
}

void app_main(void)
{
    ledc_buzzer_init();

    spi_25LC040_init(VSPI_HOST, PIN_EEPROM_CS, PIN_SPI_CLK, PIN_SPI_MOSI, PIN_SPI_MISO, CLK_SPEED_HZ, &spi_device);
    spi_25LC040_write_status(spi_device, 0x0); 

    rc522_init(true);
}

esp_err_t rc522_init(bool attach_to_bus) {
    rc522_config_t config = {
        .spi.host = VSPI_HOST,
        .spi.sda_gpio = PIN_RC55_CS,
        .spi.bus_is_initialized = attach_to_bus
    };

    if (!attach_to_bus) {
        config.spi.sck_gpio = PIN_SPI_CLK;
        config.spi.mosi_gpio = PIN_SPI_MOSI;
        config.spi.miso_gpio = PIN_SPI_MISO;
    }
    
    esp_err_t ret = rc522_create(&config, &scanner);
    if (ret != ESP_OK) {
        ESP_LOGE(RC522_TAG, "Failed to create scanner: %d", ret);
        return ret;
    }

    ret = rc522_register_events(scanner, RC522_EVENT_ANY, rc522_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(RC522_TAG, "Failed to register events: %d", ret);
        return ret;
    }

    ret = rc522_start(scanner);
    if (ret != ESP_OK) {
        ESP_LOGE(RC522_TAG, "Failed to start scanner: %d", ret);
        return ret;
    }

    return ret;
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