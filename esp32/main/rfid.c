#include <stdio.h>
#include <esp_log.h>
#include <inttypes.h>

#include "rc522.h"
#include "esp_wifi_handle.h"
#include "spi_25LC040A_eeprom.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "nvs_flash.h"

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

#define API_ENDPOINT "http://192.168.43.168/check_access"

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

void request_seq();
void access_seq();
void forb_seq();

uint64_t read_sn_eeprom(spi_device_handle_t, uint8_t);

TaskHandle_t green_led_task_handle = NULL;
TaskHandle_t red_led_task_handle = NULL;

static const char* RC522_TAG = "rc522";
static rc522_handle_t scanner;

static const char* EEPROM_TAG = "eeprom";
spi_device_handle_t spi_device;

static const char *WIFI_TAG = "wifi";

static void rc522_handler(void* arg, esp_event_base_t base, int32_t event_id, void* event_data)
{
    rc522_event_data_t* data = (rc522_event_data_t*) event_data;

    switch(event_id) {
        case RC522_EVENT_TAG_SCANNED: {
                rc522_tag_t* tag = (rc522_tag_t*) data->ptr;
                uint64_t sn = tag->serial_number;

                // print the serial number as hexadecimal
                ESP_LOG_BUFFER_HEX(RC522_TAG, &sn, sizeof(sn));

                ESP_LOGI(RC522_TAG, "Tag scanned (sn: %" PRIu64 ")", sn);

                if (request_access(sn))
                    access_seq();
                else
                    forb_seq();
            
                // store the serial number in the eeprom (black box)
                uint8_t address = 0x00;
                spi_25LC040_write_enable(spi_device);
                spi_25LC040_write_page(spi_device, address, (uint16_t*)&sn, 16);
                spi_25LC040_write_disable(spi_device);
                
                vTaskDelay(100 / portTICK_PERIOD_MS);

                // read the stored serial number from the eeprom (black box)
                uint64_t read_sn = read_sn_eeprom(spi_device, address);
                ESP_LOGI(RC522_TAG, "Stored in the black box: %" PRIu64 "", read_sn);
            }
            break;
    }
}

void app_main(void)
{
    /* buzzer */
    ledc_buzzer_init();

    /* eeprom */
    spi_25LC040_init(VSPI_HOST, PIN_EEPROM_CS, PIN_SPI_CLK, PIN_SPI_MOSI, PIN_SPI_MISO, CLK_SPEED_HZ, &spi_device);
    spi_25LC040_write_status(spi_device, 0x00); // disable write protection

    /* rc522 */
    rc522_init(true); // true - attach to spi bus

    /* wifi */
    init_nvs_partition();
    wifi_init();

    /* read content of black box */
    uint8_t address = 0x00;
    uint64_t read_sn = read_sn_eeprom(spi_device, address);
    ESP_LOGI(RC522_TAG, "Stored in the black box: %" PRIu64 "", read_sn);
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

uint64_t read_sn_eeprom(spi_device_handle_t spi_device, uint8_t address) {
    uint8_t read_data;
    uint64_t read_sn = 0;
    for (int i = 15; i >= 0; i--) {
        spi_25LC040_read_byte(spi_device, address + i, &read_data);
        read_sn |= (uint64_t) read_data << (i * 8);
    }
    return read_sn;
}

bool request_access(uint64_t serial_number) {
    request_seq();

    char sn_str[64];
    snprintf(sn_str, sizeof(sn_str), "%" PRIu64, serial_number);
    char post_data[100];
    snprintf(post_data, sizeof(post_data), "{\"sn\":\"%s\"}", sn_str);
    
    bool access = false;
    HttpRequestParams params = {
        .p_access = &access,
        .url = API_ENDPOINT,
        .post_data = post_data
    };

    xTaskCreate(&http_request_task, "http_request_task", 8192, (void*)&params, 5, NULL);
    
    // task is asynchronous, wait some time for it to finish
    vTaskDelay(500 / portTICK_PERIOD_MS);

    if (access)
        ESP_LOGI(RC522_TAG, "Access granted.");
    else
        ESP_LOGI(RC522_TAG, "Access denied.");

    return access;
}

void request_seq() {
    buzzer(1);

    vTaskDelay(300 / portTICK_PERIOD_MS);

    buzzer(0);
}

void access_seq() {
    green_on();
    buzzer(1);

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    green_off();
    buzzer(0);
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