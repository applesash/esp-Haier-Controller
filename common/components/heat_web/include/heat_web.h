#pragma once

#include "esp_err.h"
#include "heat_modbus.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HEAT_WEB_AP_SSID "haier-hmi"
#define HEAT_WEB_HTTP_PORT 80

esp_err_t heat_web_start(void);
void heat_web_set_modbus_config(const heat_modbus_config_t *config);

#ifdef __cplusplus
}
#endif
