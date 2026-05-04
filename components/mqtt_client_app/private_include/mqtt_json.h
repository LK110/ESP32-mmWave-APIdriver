#pragma once

#include <stdbool.h>
#include <stdint.h>

bool extract_json_rate_value(const char *json, const char *key, uint32_t *value_out);
