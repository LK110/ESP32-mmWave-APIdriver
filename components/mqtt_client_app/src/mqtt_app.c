#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"
#include "esp_log.h"

#include "mqtt_app.h"
#include "mqtt_event.h"
#include "mqtt_types.h"
#include "../private_include/mqtt_watchdog.h"

static const char *TAG = "* mqtt_app *";

mqtt_app_context_t g_ctx = {
    .client = NULL,
    .client_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
        .network.reconnect_timeout_ms = MQTT_RECONNECT_DELAY_MS,
        .session.disable_clean_session = true,
        .session.keepalive = MQTT_KEEPALIVE_SEC,
        .session.disable_keepalive = false,
        .session.protocol_ver = MQTT_PROTOCOL_V_5,
        .session.last_will = {
            .topic = NULL,
            .msg = "{\"status\":\"offline\"}",
            .qos = 1,
            .retain = true,
        },
    },
    .event_group = NULL,
    .hb_rate_change_cb = NULL,
    .sensor_rate_change_cb = NULL,
    .configuration_topic = NULL,
    .sensor_status_topic = NULL,
    .sensor_status_check_topic = NULL,
    .reset_topic = NULL,
    .reset_message_sent = false,
    .state_payload_seq = 0,
    .sensor_status_payload_seq = 0,
    .hb_watchdog_timer = NULL,
    .hb_dead_offset_ms = HEARTBEAT_DEAD_OFFSET_MS,
    .sensor_alive = 1,
};

void mqtt_app_start(const char *device_id,
                    const char *room_id,
                    const char *connection_status_topic,
                    const char *configuration_topic,
                    const char *sensor_status_topic,
                    const char *sensor_status_check_topic,
                    const char *reset_topic)
{
    g_ctx.device_id = device_id;
    g_ctx.room_id = room_id;

    g_ctx.connection_status_topic = connection_status_topic;
    g_ctx.configuration_topic = configuration_topic;
    g_ctx.sensor_status_topic = sensor_status_topic;
    g_ctx.sensor_status_check_topic = sensor_status_check_topic;
    g_ctx.reset_topic = reset_topic;
    g_ctx.reset_message_sent = false;

    g_ctx.client_cfg.session.last_will.topic = connection_status_topic;

    g_ctx.state_payload_seq = 0;
    g_ctx.sensor_status_payload_seq = 0;

    if (!g_ctx.event_group)
        g_ctx.event_group = xEventGroupCreate();

    if (!g_ctx.event_group)
    {
        ESP_LOGE(TAG, "Failed to create MQTT event group");
        return;
    }

    ESP_LOGW(TAG, "Connecting to broker: %s", g_ctx.client_cfg.broker.address.uri);

    g_ctx.client = esp_mqtt_client_init(&g_ctx.client_cfg);
    if (!g_ctx.client)
    {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return;
    }

    // create heartbeat watchdog timer
    {
        uint32_t hb_interval_ms = get_heartbeat_interval_ms();
        uint32_t period_ms = 3U * hb_interval_ms + g_ctx.hb_dead_offset_ms; // Period = (3 * hb_interval) + offset
        g_ctx.hb_watchdog_timer = xTimerCreate("hb_watchdog", pdMS_TO_TICKS(period_ms), pdFALSE, NULL, hb_watchdog_timer_cb);
        if (g_ctx.hb_watchdog_timer)
            xTimerStart(g_ctx.hb_watchdog_timer, 0);
        else
            ESP_LOGW(TAG, "Failed to create heartbeat watchdog timer");
    }

#ifdef CONFIG_MQTT_PROTOCOL_5
    esp_mqtt5_connection_property_config_t connect_props = {
        .session_expiry_interval = MQTT_SESSION_EXPIRY_SEC,
        .will_delay_interval = MQTT_LWT_WILL_DELAY_SEC,
    };

    if (esp_mqtt5_client_set_connect_property(g_ctx.client, &connect_props) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set MQTT5 connect properties");
        return;
    }
#endif

    esp_mqtt_client_register_event(g_ctx.client, ESP_EVENT_ANY_ID, mqtt_app_event_handler, &g_ctx);
    esp_mqtt_client_start(g_ctx.client);
}

void mqtt_app_register_rate_callbacks(mqtt_hb_rate_change_cb_t hb_cb,
                                      mqtt_sensor_rate_change_cb_t sensor_cb)
{
    g_ctx.hb_rate_change_cb = hb_cb;
    g_ctx.sensor_rate_change_cb = sensor_cb;
}

void mqtt_app_wait_connected(TickType_t timeout_ticks)
{
    if (!g_ctx.event_group)
        return;

    xEventGroupWaitBits(
        g_ctx.event_group,
        MQTT_CONNECTED_BIT,
        pdFALSE,
        pdTRUE,
        timeout_ticks);
}

bool mqtt_app_is_connected(void)
{
    if (!g_ctx.event_group)
        return false;
    return (xEventGroupGetBits(g_ctx.event_group) & MQTT_CONNECTED_BIT) != 0;
}
