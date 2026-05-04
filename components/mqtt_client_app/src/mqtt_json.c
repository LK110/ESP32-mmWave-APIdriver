#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "../private_include/mqtt_json.h"
#include "cJSON.h"

bool extract_json_rate_value(const char *json, const char *key, uint32_t *value_out)
{
    if (!json || !key || !value_out)
        return false;

    cJSON *root = cJSON_Parse(json);
    if (!root)
        return false;

    cJSON *item = cJSON_GetObjectItemCaseSensitive(root, key);
    if (!item)
    {
        cJSON_Delete(root);
        return false;
    }

    if (cJSON_IsNumber(item))
    {
        double d = item->valuedouble;
        if (d < 0.0 || d > (double)UINT32_MAX)
        {
            cJSON_Delete(root);
            return false;
        }
        *value_out = (uint32_t)d;
        cJSON_Delete(root);
        return true;
    }

    if (cJSON_IsString(item) && (item->valuestring != NULL))
    {
        const char *s = item->valuestring;
        if (*s == '\0')
        {
            cJSON_Delete(root);
            return false;
        }

        uint64_t v = 0;
        for (const char *p = s; *p != '\0'; ++p)
        {
            if (*p < '0' || *p > '9')
            {
                cJSON_Delete(root);
                return false;
            }
            uint32_t digit = (uint32_t)(*p - '0');
            if (v > (UINT32_MAX - digit) / 10ULL)
            {
                cJSON_Delete(root);
                return false;
            }
            v = v * 10ULL + digit;
        }
        *value_out = (uint32_t)v;
        cJSON_Delete(root);
        return true;
    }

    cJSON_Delete(root);
    return false;
}
