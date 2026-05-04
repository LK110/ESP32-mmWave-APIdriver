#pragma once

#include <stdint.h>
#include "freertos/timers.h"

void mqtt_app_heartbeat_seen(void);
void mqtt_app_update_watchdog_interval(uint32_t interval_ms);
void hb_watchdog_timer_cb(TimerHandle_t xTimer);
