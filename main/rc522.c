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

esp_err_t rc522_register_events(rc522_handle_t rc522, rc522_event_t event, rc522_event_handler_t handler, void* handler_args) {
    return esp_event_handler_register_with(rc522->event_loop_handle, RC522_EVENT, event, handler, handler_args);
}

esp_err_t rc522_unregister_events(rc522_handle_t rc522, rc522_event_t event, rc522_event_handler_t handler) {
    return esp_event_handler_unregister_with(rc522->event_loop_handle, RC522_EVENT, event, handler);
}