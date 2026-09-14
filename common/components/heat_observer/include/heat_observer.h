#pragma once

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uart_port_t uart_port;
    gpio_num_t tx_gpio;
    gpio_num_t rx_gpio;
    uint32_t baud_rate;
} heat_observer_config_t;

esp_err_t heat_observer_start(const heat_observer_config_t *config);

#ifdef __cplusplus
}
#endif
