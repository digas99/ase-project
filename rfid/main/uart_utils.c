#include <stdio.h>

#include "driver/uart.h"

// UART setup config:
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/uart.html

int setup_uart();
char get_char();
int get_string(char*);

int setup_uart() {
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    // check issue E (325) uart: uart_param_config(748): Invalid src_clk
    uart_param_config(UART_NUM_0, &uart_config);
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, 1024, 0, 0, NULL, 0));

    return 0;
}

char get_char() {
    // flush stdout to make sure prints before this function are printed
    fflush(stdout);

    char data[1];
    uint32_t len = 0;
    while (1) {
        len = uart_read_bytes(UART_NUM_0, (uint8_t*)data, 1, 100);

        // new line is being sent as 0x0d (CR) instead of 0x0a (LF)
        if (data[0] == 0x0d) data[0] = 0x0a;

        // print char back to terminal for readability purposes
        // only print and return when there is something there
        if (len > 0) {
            printf("%c", data[0]);
            return data[0];
        }
    }
    
    return '\0';
}

int get_string(char *data) {
    char c;
    uint32_t i = 0;
    while ((c = get_char()) != '\n') {
        data[i++] = c;
    }
    // add null terminator
    data[i] = '\0';

    // print newline for readability purposes
    printf("\n");

    return 0;
}