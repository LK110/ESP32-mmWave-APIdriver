#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"
#include "mqtt_client.h"

#define MQTT_CONNECTED_BIT (1 << 0)

#define MQTT_RECONNECT_DELAY_MS 3000
#define MQTT_KEEPALIVE_SEC 10
#define MQTT_LWT_WILL_DELAY_SEC 10 // gives X seconds for the recconect before the LWT message is published
#define MQTT_SESSION_EXPIRY_SEC 3600

#define MQTT_BROKER_URI "mqtt://" MQTT_BROKER_IP ":1883" // MQTT_BROKER_IP is loaded at compile time

typedef void (*mqtt_hb_rate_change_cb_t)(uint32_t interval_ms);
typedef void (*mqtt_sensor_rate_change_cb_t)(uint32_t interval_ms);

typedef struct
{
    esp_mqtt_client_handle_t client;
    esp_mqtt_client_config_t client_cfg;

    EventGroupHandle_t event_group;

    const char *device_id;
    const char *room_id;
    mqtt_hb_rate_change_cb_t hb_rate_change_cb;
    mqtt_sensor_rate_change_cb_t sensor_rate_change_cb;

    const char *connection_status_topic;
    const char *configuration_topic;
    const char *sensor_status_topic;
    const char *sensor_status_check_topic;
    const char *reset_topic;
    bool reset_message_sent;

    uint32_t state_payload_seq;
    uint32_t sensor_status_payload_seq;
    /* Heartbeat watchdog */
    TimerHandle_t hb_watchdog_timer;
    uint32_t hb_dead_offset_ms;
    uint8_t sensor_alive; // 1 = alive, 0 = dead
} mqtt_app_context_t;
