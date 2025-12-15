#include <stdint.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <esp_err.h>

#include "mr24hpc.h"
#include "mr24hpc_uart.h"
#include "internal.h"


static QueueHandle_t uart_rx_queue;


esp_err_t mr24hpc_init(void) {
    mr24hpc_uart_init();
    mr24hpc_parser_init();
    return ESP_OK;
}

esp_err_t mr24hpc_start(void) {
    uart_rx_queue = xQueueCreate(256, sizeof(uint8_t));
    xTaskCreate(mr24hpc_uart_task, "mr24_uart", 2048, NULL, 10, NULL);
    xTaskCreate(mr24hpc_driver_task, "mr24_drv", 4096, NULL, 9, NULL);
    return ESP_OK;
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