#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "rc522.h"

struct rc522 {
    rc522_config_t config;
    TASK_HANDLE_T task_handle;
    spi_device_handle_t spi_handle;
    esp_event_loop_handle_t event_loop_handle;
    bool busy; // "tag_was_present_last_time"
}

ESP_EVENT_DEFINE_BASE(RC522_EVENT);

esp_err_t rc522_create(rc522_config_t *config, rc522_handle_t *rc522_output) {
    if (!config || !rc522)
        return ESP_ERR_INVALID_ARG;

    esp_err_t ret;

    rc522_handle_t handle = calloc(1, sizeof(struct rc522));
    if (!handle)
        return ESP_ERR_NO_MEM;

    handle->config = *config;
    
    // manage SPI bus
    spi_device_interface_config_t device_config = {
        .click_speed_hz = config->clk_freq,
        .mode = 0,
        .spics_io_num = config->spi_ss_io,
        .queue_size = 7,
    };

    spi_bus_config_t bus_config = {
        .miso_io_num = config->spi_miso_io,
        .mosi_io_num = config->spi_mosi_io,
        .sclk_io_num = config->spi_sck_io,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    ret = spi_bus_initialize(config->spi_host, &bus_config, 0);
    if (ret != ESP_OK)
        return ret;

    ret = spi_bus_add_device(config->spi_host, &device_config, &rc522->spi_handle);
    if (ret != ESP_OK)
        return ret;

    esp_event_loop_args_t event_args = {
        .queue_size = 1,
        .task_name = NULL,
    };

    ret = esp_event_loop_create(&event_args, &rc522->event_loop_handle);
    if (ret != ESP_OK)
        return ret;

    xTaskCreate(rc522_task, "rc522_task", config->task_stack_size, rc522, config->task_priority, &rc522->task_handle);
    *rc522_output = rc522;
    return ESP_OK;
}

esp_err_t rc522_register_events(rc522_handle_t rc522, rc522_event_t event, rc522_event_handler_t handler, void* handler_args) {
    return esp_event_handler_register_with(rc522->event_loop_handle, RC522_EVENT, event, handler, handler_args);
}

esp_err_t rc522_unregister_events(rc522_handle_t rc522, rc522_event_t event, rc522_event_handler_t handler) {
    return esp_event_handler_unregister_with(rc522->event_loop_handle, RC522_EVENT, event, handler);
}

esp_err_t rc522_start(rc522_handle_t rc522) {
    esp_err_t err;

    
}