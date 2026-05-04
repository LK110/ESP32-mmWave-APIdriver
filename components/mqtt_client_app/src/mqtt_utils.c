#include <string.h>

#include "../private_include/mqtt_utils.h"

bool topic_matches(const char *topic, size_t topic_len, const char *expected_topic)
{
    if (!topic || !expected_topic)
        return false;

    size_t expected_len = strlen(expected_topic);

    return (topic_len == expected_len) && (strncmp(topic, expected_topic, topic_len) == 0);
}

bool copy_payload(char *dst, size_t dst_size, const char *data, int d_len)
{
    if (!dst || !data || d_len <= 0 || (size_t)d_len >= dst_size)
        return false;

    memcpy(dst, data, (size_t)d_len);
    dst[d_len] = '\0';
    return true;
}
