#pragma once
#include "driver/spi_master.h"

ESP_EVENT_DECLARE_BASE(RC522_EVENTS);

typedef struct rc522* rc522_handle_t;

typedef struct {
    uint16_t scan_interval_ms;  /* how fast scanning of tags is done */
    size_t task_stack_size;     /* stack size of the task */
    uint8_t task_priority;      /* priority of the task */
    
    // SPI
    spi_host_device_t spi_host; /* SPI host device */
    int spi_miso_io;            /* MISO GPIO pin */
    int spi_mosi_io;            /* MOSI GPIO pin */
    int spi_sck_io;             /* SCK GPIO pin */
    int spi_ss_io;              /* SS GPIO pin */
    int clk_freq;               /* SPI clock frequency */
} rc522_config_t;

enum {
    RC522_EVENT_TAG_SCANNED,
};

typedef struct {
    rc522_handle_t rc522;
    void* ptr;
} rc522_event_data_t;

esp_err_t rc522_create(rc522_config_t *config, rc522_handle_t *rc522);

esp_err_t rc522_register_events(rc522_handle_t rc522, esp_event_base_t event, esp_event_handler_t handler, void *handler_args);

esp_err_t rc522_unregister_events(rc522_handle_t rc522, esp_event_base_t event, esp_event_handler_t handler);

esp_err_t rc522_start(rc522_handle_t rc522);