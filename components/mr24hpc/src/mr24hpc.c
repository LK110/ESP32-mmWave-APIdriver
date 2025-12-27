#include <stdint.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_timer.h"
#include "esp_err.h"
#include "freertos/semphr.h"

#include "mr24hpc.h"
#include "mr24hpc_uart.h"
#include "internal.h"

static void mr24hpc_uart_task(void *arg);
static void mr24hpc_driver_task(void *arg);
uint8_t calculate_checksum(const uint8_t *data, size_t len);

static QueueHandle_t uart_rx_queue = NULL;
static mr24hpc_state_t global_senstor_state;
static SemaphoreHandle_t state_mutex = NULL;
static mr24hpc_callback_function state_update_callback_function = NULL;


// ==================== Initialization ====================

esp_err_t mr24hpc_init(void) {
    mr24hpc_uart_init();
    uart_rx_queue = xQueueCreate(256, sizeof(uint8_t));
    mr24hpc_parser_init();
    state_mutex = xSemaphoreCreateMutex();
    return ESP_OK;
}

esp_err_t mr24hpc_start(void) {
    mr24hpc_activate_underlying_open_functions();
    xTaskCreate(mr24hpc_uart_task, "mr24_uart", 2048, NULL, 10, NULL);
    xTaskCreate(mr24hpc_driver_task, "mr24_drv", 4096, NULL, 9, NULL);
    return ESP_OK;
}

esp_err_t mr24hpc_activate_underlying_open_functions(void) {
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
    
    // try several times
    for (int i=0; i<10; i++) {
        mr24hpc_uart_write(check_package, sizeof(check_package));
        read_len = mr24hpc_uart_read(response, sizeof(response), 1000);

        if (read_len == sizeof(expected_response) &&
            memcmp(response, expected_response, sizeof(expected_response)) == 0) {
            return ESP_OK;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
    // failed to activate
    return ESP_FAIL;
}


// ==================== State Management ====================

bool mr24hpc_get_state(mr24hpc_state_t *state_copy) {
    if (!state_copy) return false;

    mr24hpc_state_lock();
    *state_copy = global_senstor_state;
    mr24hpc_state_unlock();

    return true;
}

esp_err_t mr24hpc_register_callback(mr24hpc_callback_function cb_function) {
    // if (!state_mutex) return ESP_FAIL;
    mr24hpc_state_lock();
    state_update_callback_function = cb_function;
    mr24hpc_state_unlock();
    return ESP_OK;
}

//iz internal.h
void mr24hpc_update_state(const mr24hpc_state_t *delta) {
    
    if (!delta) return;

    mr24hpc_state_t new_state_snapshot;
    mr24hpc_callback_function cb_to_call = NULL;

    mr24hpc_state_lock();

    if (delta->valid_mask & MR24HPC_VALID_PRESENCE) {
        global_senstor_state.presence = delta->presence;
    }

    if (delta->valid_mask & MR24HPC_VALID_MOTION_STATE) {
        global_senstor_state.motion_state = delta->motion_state;
    }

    if (delta->valid_mask & MR24HPC_VALID_DISTANCE) {
        global_senstor_state.distance_m = delta->distance_m;
    }

    if (delta->valid_mask & MR24HPC_VALID_SPEED) {
        global_senstor_state.speed_m_s = delta->speed_m_s;
    }

    if (delta->valid_mask & MR24HPC_VALID_BODY_SIGNALS) {
        global_senstor_state.body_signals = delta->body_signals;
    }

    global_senstor_state.last_update_ms = delta->last_update_ms;

    new_state_snapshot = global_senstor_state;
    cb_to_call = state_update_callback_function;

    mr24hpc_state_unlock();

    if (cb_to_call) {
        cb_to_call(&new_state_snapshot);
    }
}

uint32_t mr24hpc_ms_since_last_update(void) {
    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000ULL);
    return now - global_senstor_state.last_update_ms;
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


// ================ semafori ================

void mr24hpc_state_lock(void) {
    xSemaphoreTake(state_mutex, portMAX_DELAY);
}

void mr24hpc_state_unlock(void) {
    xSemaphoreGive(state_mutex);
}