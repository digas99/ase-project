#pragma once

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_netif.h"

#include "esp_http_client.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#define WIFI_SSID "HUAWEI P smart 2019"
#define WIFI_PASSWORD "diogocorreia99"

typedef struct {
    bool* p_access;
    const char* url;
    const char* post_data;
} HttpRequestParams;

void wifi_init(void);

void http_request_task(void *pvParameters);

void init_nvs_partition(void);