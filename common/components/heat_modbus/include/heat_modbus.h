#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "esp_err.h"
#include "driver/uart.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uart_port_t uart_port;
    gpio_num_t tx_gpio;
    gpio_num_t rx_gpio;
    uint32_t baud_rate;
    uint32_t response_timeout_ms;
} heat_modbus_config_t;

typedef struct {
    uart_port_t uart_port;
    uint8_t slave_address;
    uint32_t response_timeout_ms;
} heat_modbus_master_t;

esp_err_t heat_modbus_master_init(heat_modbus_master_t *master,
                                  const heat_modbus_config_t *config);
esp_err_t heat_modbus_read_holding_registers(heat_modbus_master_t *master,
                                              uint8_t slave_address,
                                              uint16_t register_address,
                                              uint16_t register_count,
                                              uint16_t *registers);
esp_err_t heat_modbus_read_input_registers(heat_modbus_master_t *master,
                                            uint8_t slave_address,
                                            uint16_t register_address,
                                            uint16_t register_count,
                                            uint16_t *registers);
esp_err_t heat_modbus_write_single_register(heat_modbus_master_t *master,
                                             uint8_t slave_address,
                                             uint16_t register_address,
                                             uint16_t value);
esp_err_t heat_modbus_write_multiple_registers(heat_modbus_master_t *master,
                                               uint8_t slave_address,
                                               uint16_t register_address,
                                               uint16_t register_count,
                                               const uint16_t *registers);

#ifdef __cplusplus
}
#endif
