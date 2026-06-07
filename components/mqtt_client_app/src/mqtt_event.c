#include <stdbool.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"

#include "mr24hpc.h"
#include "mqtt_app.h"
#include "mqtt_event.h"
#include "../private_include/mqtt_json.h"
#include "../private_include/mqtt_utils.h"

static const char *TAG = "* mqtt_app *";

// ==================== Forward Declarations ====================

static void process_configuration_message(mqtt_app_context_t *ctx,
                                          const char *topic,
                                          size_t topic_len,
                                          const char *data,
                                          int data_len);
static void process_check_connection_status_message(mqtt_app_context_t *ctx,
                                                    const char *topic,
                                                    size_t topic_len,
                                                    const char *data,
                                                    int data_len);

// ==================== Event Handler ====================

void mqtt_app_event_handler(void *handler_args,
                            esp_event_base_t base,
                            int32_t event_id,
                            void *event_data)
{
    (void)base; // unused parameter, part of the required signature for esp_event_handler_t

    mqtt_app_context_t *ctx = (mqtt_app_context_t *)handler_args;
    esp_mqtt_event_handle_t event = event_data;

    if (!ctx || !event)
        return;

    esp_mqtt_client_handle_t client = event->client;

    switch (event_id)
    {
    case MQTT_EVENT_CONNECTED:
        if (ctx->event_group)
            xEventGroupSetBits(ctx->event_group, MQTT_CONNECTED_BIT);
        ESP_LOGW(TAG, "Client connected to broker: %s", ctx->client_cfg.broker.address.uri);

        // publish retained "online" message to LWT topic upon successful connection
        esp_mqtt_client_publish(client, ctx->connection_status_topic, "{\"status\":\"online\"}", 0, 1, 1); // QoS 1, retain=true
        ESP_LOGI(TAG, "Published connection status: {\"status\":\"online\"}");

        // send reset message on connect (e.g. after a reboot)
        if (!ctx->reset_message_sent && ctx->reset_topic)
        {
            uint32_t sensor_rate_ms = get_sensor_rate_interval_ms();
            uint32_t hb_rate_ms = get_heartbeat_interval_ms();
            mqtt_app_publish_reset(ctx->reset_topic, hb_rate_ms, sensor_rate_ms);
            ctx->reset_message_sent = true;
        }

        // subscribe to topics after connecting
        if (ctx->configuration_topic)
            esp_mqtt_client_subscribe_single(client, ctx->configuration_topic, 1); // QoS 1
        if (ctx->sensor_status_check_topic)
            esp_mqtt_client_subscribe_single(client, ctx->sensor_status_check_topic, 1); // QoS 1
        break;

    case MQTT_EVENT_DISCONNECTED:
        if (ctx->event_group)
            xEventGroupClearBits(ctx->event_group, MQTT_CONNECTED_BIT);
        ESP_LOGW(TAG, "Disconnected, auto-reconnect in %d ms...", MQTT_RECONNECT_DELAY_MS);
        break;

    case MQTT_EVENT_DATA:
        const char *topic = event->topic;
        const char *data = event->data;
        int t_len = event->topic_len;
        int d_len = event->data_len;

        if (!topic || !data)
            break;

        ESP_LOGI(TAG, "received message: (%.*s) %.*s", t_len, topic, d_len, data);

        process_configuration_message(ctx, topic, (size_t)t_len, data, d_len);
        process_check_connection_status_message(ctx, topic, (size_t)t_len, data, d_len);
        break;

    default:
        break;
    }
}

static void process_configuration_message(mqtt_app_context_t *ctx,
                                          const char *topic,
                                          size_t topic_len,
                                          const char *data,
                                          int data_len)
{
    if (!ctx || !topic_matches(topic, topic_len, ctx->configuration_topic))
        return;

    char payload[192];
    if (!copy_payload(payload, sizeof(payload), data, data_len))
        return;

    uint32_t hb_rate_ms = 0;
    if (extract_json_rate_value(payload, "hb_rate", &hb_rate_ms) && hb_rate_ms > 0)
    {
        if (ctx->hb_rate_change_cb)
        {
            ESP_LOGI(TAG, "--> received hb rate change command: %" PRIu32 " ms", hb_rate_ms);
            ctx->hb_rate_change_cb(hb_rate_ms);
        }
    }

    uint32_t sensor_rate_ms = 0;
    if (extract_json_rate_value(payload, "sensor_rate", &sensor_rate_ms) && sensor_rate_ms > 0)
    {
        if (ctx->sensor_rate_change_cb)
        {
            ESP_LOGI(TAG, "--> received sensor rate change command: %" PRIu32 " ms", sensor_rate_ms);
            ctx->sensor_rate_change_cb(sensor_rate_ms);
        }
    }
}

static void process_check_connection_status_message(mqtt_app_context_t *ctx,
                                                    const char *topic,
                                                    size_t topic_len,
                                                    const char *data,
                                                    int data_len)
{
    if (!ctx || !topic_matches(topic, topic_len, ctx->sensor_status_check_topic))
        return;

    char payload[192];
    if (!copy_payload(payload, sizeof(payload), data, data_len) || strcmp(payload, "{\"check sensor\"}") != 0)
        return;

    ESP_LOGI(TAG, "--> received check sensor status command");

    trigger_heartbeat_now();
}
