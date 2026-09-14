#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HEAT_WEB_AP_SSID "haier-hmi"
#define HEAT_WEB_HTTP_PORT 80

esp_err_t heat_web_start(void);

#ifdef __cplusplus
}
#endif
