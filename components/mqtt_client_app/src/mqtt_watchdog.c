#include "esp_log.h"

#include "mqtt_app.h"
#include "../private_include/mqtt_watchdog.h"

static const char *TAG = "* mqtt_watchdog *";

void mqtt_app_heartbeat_seen(void)
{
    g_ctx.sensor_alive = 1;
    if (g_ctx.hb_watchdog_timer)
    {
        xTimerReset(g_ctx.hb_watchdog_timer, 0);
    }
}

void mqtt_app_update_watchdog_interval(uint32_t interval_ms)
{
    if (!g_ctx.hb_watchdog_timer)
        return;

    uint32_t period_ms = 3U * interval_ms + g_ctx.hb_dead_offset_ms;
    xTimerChangePeriod(g_ctx.hb_watchdog_timer, pdMS_TO_TICKS(period_ms), 0);
    xTimerReset(g_ctx.hb_watchdog_timer, 0);
}

void hb_watchdog_timer_cb(TimerHandle_t xTimer)
{
    (void)xTimer;
    g_ctx.sensor_alive = 0;

    if (g_ctx.client && mqtt_app_is_connected() && g_ctx.sensor_status_topic)
    {
        mqtt_app_publish_sensor_status(g_ctx.sensor_status_topic, "dead", get_heartbeat_interval_ms());
    }
}
