#pragma once

#include <stdbool.h>
#include <stddef.h>

bool topic_matches(const char *topic, size_t topic_len, const char *expected_topic);
bool copy_payload(char *dst, size_t dst_size, const char *data, int d_len);
