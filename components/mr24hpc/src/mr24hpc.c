#include <stdint.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <esp_err.h>

#include "mr24hpc.h"
#include "mr24hpc_uart.h"
#include "internal.h"


static QueueHandle_t uart_rx_queue = NULL;


esp_err_t mr24hpc_init(void) {
    mr24hpc_uart_init();
    mr24hpc_parser_init();
    return ESP_OK;
}

esp_err_t mr24hpc_start(void) {
    uart_rx_queue = xQueueCreate(256, sizeof(uint8_t));
    mr24hpc_activate_underlying_open_functions();
    xTaskCreate(mr24hpc_uart_task, "mr24_uart", 2048, NULL, 10, NULL);
    xTaskCreate(mr24hpc_driver_task, "mr24_drv", 4096, NULL, 9, NULL);
    return ESP_OK;
}

void mr24hpc_activate_underlying_open_functions(void) {
    //send configuration package to sensor
    uint8_t package[] =  {0x53,0x59,0x08,0x00,0x00,0x01,0x01,0xFF,0x54,0x43};
    package[7] = calculate_checksum(package, 7);
    mr24hpc_uart_write(package, sizeof(package));

    //check if functions are activated
    uint8_t check_package[] = {0x53,0x59,0x08,0x80,0x00,0x01,0x0F,0xFF,0x54,0x43};
    check_package[7] = calculate_checksum(check_package, 7);

    uint8_t expected_response[] = {0x53,0x59,0x08,0x80,0x00,0x01,0x01,0xFF,0x54,0x43};
    expected_response[7] = calculate_checksum(expected_response, 7);
    uint8_t response[10];
    int read_len;
    
    for (;;) {
        mr24hpc_uart_write(check_package, sizeof(check_package));
        read_len = mr24hpc_uart_read(response, sizeof(response), 1000);

        if (read_len == sizeof(expected_response) &&
                memcmp(response, expected_response, sizeof(expected_response)) == 0) break;

        vTaskDelay(pdMS_TO_TICKS(100));
    }

    return;
}

// ==================== Task Implementations ====================

static void mr24hpc_uart_task(void *arg) {
    uint8_t byte;

    while (1) {
        if (mr24hpc_uart_read(&byte, 1, 100) == 1) {
            xQueueSend(uart_rx_queue, &byte, portMAX_DELAY);
        }
    }
}

static void mr24hpc_driver_task(void *arg) {
    uint8_t byte;

    while (1) {
        if (xQueueReceive(uart_rx_queue, &byte, portMAX_DELAY)) {
            mr24hpc_parser_feed(byte);
        }
    }
}

// pomocne funkcije
uint8_t calculate_checksum(const uint8_t *data, size_t len) {
    uint32_t sum = 0;
    for (int i = 0; i < len; ++i) sum += data[i];
    return (uint8_t)(sum & 0xFF);
}