#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>
#include <limits.h>

#include "esp_log.h"
#include "mqtt_app.h"

static const char *TAG = "* mqtt_publish *";

static const char *uof_direction_to_str(UOF_mr24hpc_direction_t dir)
{
    switch (dir)
    {
    case UOF_MR24HPC_DIR_APPROACHING:
        return "APPROACHING";
    case UOF_MR24HPC_DIR_MOVING_AWAY:
        return "MOVING AWAY";
    default:
        return "NONE/STATIONARY";
    }
}

// thread-safe equivalent to g_payload_seq++
static uint32_t next_state_payload_seq(void)
{
    return __atomic_fetch_add(&g_ctx.state_payload_seq, 1U, __ATOMIC_RELAXED);
}

static uint32_t next_sensor_status_payload_seq(void)
{
    return __atomic_fetch_add(&g_ctx.sensor_status_payload_seq, 1U, __ATOMIC_RELAXED);
}

static uint32_t normalize_seq_after_overflow(uint32_t seq)
{
    if (seq != UINT32_MAX)
        return seq;

    __atomic_store_n(&g_ctx.state_payload_seq, 0U, __ATOMIC_RELAXED);
    __atomic_store_n(&g_ctx.sensor_status_payload_seq, 0U, __ATOMIC_RELAXED);

    ESP_LOGW(TAG, "SEQ overflow detected, resetting seq counters to 0");

    if (g_ctx.reset_topic)
    {
        uint32_t sensor_rate_ms = get_sensor_rate_interval_ms();
        uint32_t hb_rate_ms = get_heartbeat_interval_ms();
        mqtt_app_publish_reset(g_ctx.reset_topic, hb_rate_ms, sensor_rate_ms);
    }

    return 0U;
}

void mqtt_app_publish_sensor_status(const char *topic, const char *status, uint32_t hb_rate)
{
    if (!g_ctx.client || !mqtt_app_is_connected() || !status || !topic)
        return;

    uint32_t seq = normalize_seq_after_overflow(next_sensor_status_payload_seq());

    char payload[192];
    snprintf(payload, sizeof(payload),
             "{"
             "\"seq\":%" PRIu32 ","
             "\"sensor\":\"%s\","
             "\"hb_rate\": %u"
             "}",
             seq,
             status,
             (unsigned)hb_rate);

    esp_mqtt_client_publish(g_ctx.client, topic, payload, 0, 0, 0); // QoS 0
    ESP_LOGI(TAG, "Published sensor status: %s", payload);
}

void mqtt_app_publish_state(const char *topic, const mr24hpc_state_t *state)
{
    if (!g_ctx.client || !mqtt_app_is_connected() || !state || !topic)
        return;

    uint32_t seq = normalize_seq_after_overflow(next_state_payload_seq());

    char payload[192];
    snprintf(payload, sizeof(payload),
             "{"
             "\"seq\":%" PRIu32 ","
             "\"presence\":%u,"
             "\"motion\":%u,"
             "\"sensor_rate\":%u"
             "}",
             seq,
             (unsigned)state->presence,
             (unsigned)state->motion,
             (unsigned)get_sensor_rate_interval_ms());

    esp_mqtt_client_publish(g_ctx.client, topic, payload, 0, 0, 0); // QoS 0
    ESP_LOGI(TAG, "Published state: %s", payload);
}

void mqtt_app_publish_uof_state(const char *topic, const UOF_mr24hpc_state_t *state)
{
    if (!g_ctx.client || !mqtt_app_is_connected() || !state || !topic)
        return;

    uint32_t seq = normalize_seq_after_overflow(next_state_payload_seq());

    char payload[288];
    snprintf(payload, sizeof(payload),
             "{"
             "\"seq\":%" PRIu32 ","
             "\"existence_energy\":%u,"
             "\"static_distance_m\":%.1f,"
             "\"motion_energy\":%u,"
             "\"motion_distance_m\":%.1f,"
             "\"motion_speed_m_s\":%.1f,"
             "\"direction\":\"%s\","
             "\"moving_params\":%u"
             "}",
             seq,
             state->existence_energy,
             state->static_distance_m,
             state->motion_energy,
             state->motion_distance_m,
             state->motion_speed_m_s,
             uof_direction_to_str(state->direction),
             state->moving_params);

    esp_mqtt_client_publish(g_ctx.client, topic, payload, 0, 0, 0);
}

void mqtt_app_publish_reset(const char *topic, uint32_t hb_rate, uint32_t sensor_rate)
{
    if (!g_ctx.client || !mqtt_app_is_connected() || !topic)
        return;

    char payload[128];
    snprintf(payload,
             sizeof(payload),
             "{"
             "\"sensor_rate\":%" PRIu32 ","
             "\"hb_rate\":%" PRIu32 ","
             "\"state_seq\":0,"
             "\"sensor_seq\":0"
             "}",
             sensor_rate,
             hb_rate);

    esp_mqtt_client_publish(g_ctx.client, topic, payload, 0, 1, 0); // QoS 1, retain=false
    ESP_LOGW(TAG, "Published reset message: %s", payload);
}
