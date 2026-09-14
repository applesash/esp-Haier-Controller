#include "heat_modbus.h"

#include <string.h>

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define HEAT_MODBUS_MAX_PDU 253
#define HEAT_MODBUS_MAX_REGISTERS 123

static const char *TAG = "heat_modbus";

static uint16_t modbus_crc16(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t index = 0; index < length; ++index) {
        crc ^= data[index];
        for (uint8_t bit = 0; bit < 8; ++bit) {
            crc = (crc & 1) != 0 ? (crc >> 1) ^ 0xA001 : crc >> 1;
        }
    }
    return crc;
}

static esp_err_t exchange(heat_modbus_master_t *master, const uint8_t *request,
                          size_t request_length, uint8_t *response, size_t response_capacity,
                          size_t *response_length) {
    if (master == NULL || request == NULL || response == NULL || response_length == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uart_flush_input(master->uart_port);
    int written = uart_write_bytes(master->uart_port, request, request_length);
    if (written != (int)request_length) {
        return ESP_FAIL;
    }
    ESP_RETURN_ON_ERROR(uart_wait_tx_done(master->uart_port, pdMS_TO_TICKS(1000)), TAG,
                        "Modbus transmit failed");

    int received = uart_read_bytes(master->uart_port, response, response_capacity,
                                   pdMS_TO_TICKS(master->response_timeout_ms));
    if (received < 5) {
        return ESP_ERR_TIMEOUT;
    }
    *response_length = (size_t)received;

    uint16_t expected_crc = modbus_crc16(response, *response_length - 2);
    uint16_t received_crc = (uint16_t)response[*response_length - 2] |
                            ((uint16_t)response[*response_length - 1] << 8);
    if (expected_crc != received_crc) {
        return ESP_ERR_INVALID_CRC;
    }
    if (response[0] != request[0]) {
        return ESP_ERR_INVALID_RESPONSE;
    }
    if ((response[1] & 0x80) != 0) {
        ESP_LOGW(TAG, "Slave %u returned exception 0x%02x", response[0], response[2]);
        return ESP_ERR_INVALID_RESPONSE;
    }
    return ESP_OK;
}

static void append_crc(uint8_t *frame, size_t length_without_crc) {
    uint16_t crc = modbus_crc16(frame, length_without_crc);
    frame[length_without_crc] = (uint8_t)(crc & 0xFF);
    frame[length_without_crc + 1] = (uint8_t)(crc >> 8);
}

esp_err_t heat_modbus_master_init(heat_modbus_master_t *master,
                                  const heat_modbus_config_t *config) {
    if (master == NULL || config == NULL || config->tx_gpio == GPIO_NUM_NC ||
        config->rx_gpio == GPIO_NUM_NC || config->baud_rate == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    uart_config_t uart_config = {
        .baud_rate = (int)config->baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_RETURN_ON_ERROR(uart_driver_install(config->uart_port, 512, 512, 0, NULL, 0), TAG,
                        "Failed to install RS485 UART");
    ESP_RETURN_ON_ERROR(uart_param_config(config->uart_port, &uart_config), TAG,
                        "Failed to configure RS485 UART");
    ESP_RETURN_ON_ERROR(uart_set_pin(config->uart_port, config->tx_gpio, config->rx_gpio,
                                     UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE), TAG,
                        "Failed to assign RS485 pins");

    master->uart_port = config->uart_port;
    master->slave_address = 1;
    master->response_timeout_ms = config->response_timeout_ms != 0
                                      ? config->response_timeout_ms
                                      : 200;
    return ESP_OK;
}

esp_err_t heat_modbus_read_holding_registers(heat_modbus_master_t *master,
                                              uint8_t slave_address,
                                              uint16_t register_address,
                                              uint16_t register_count,
                                              uint16_t *registers) {
    if (master == NULL || registers == NULL || register_count == 0 ||
        register_count > HEAT_MODBUS_MAX_REGISTERS) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t request[8] = {slave_address, 0x03,
                          (uint8_t)(register_address >> 8), (uint8_t)register_address,
                          (uint8_t)(register_count >> 8), (uint8_t)register_count};
    append_crc(request, 6);
    uint8_t response[HEAT_MODBUS_MAX_PDU + 2];
    size_t response_length = 0;
    esp_err_t err = exchange(master, request, sizeof(request), response, sizeof(response),
                             &response_length);
    if (err != ESP_OK) {
        return err;
    }
    if (response[2] != register_count * 2 || response_length < 5 + register_count * 2) {
        return ESP_ERR_INVALID_RESPONSE;
    }
    for (uint16_t index = 0; index < register_count; ++index) {
        registers[index] = ((uint16_t)response[3 + index * 2] << 8) | response[4 + index * 2];
    }
    return ESP_OK;
}

esp_err_t heat_modbus_read_input_registers(heat_modbus_master_t *master,
                                            uint8_t slave_address,
                                            uint16_t register_address,
                                            uint16_t register_count,
                                            uint16_t *registers) {
    if (master == NULL || registers == NULL || register_count == 0 ||
        register_count > HEAT_MODBUS_MAX_REGISTERS) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t request[8] = {slave_address, 0x04,
                          (uint8_t)(register_address >> 8), (uint8_t)register_address,
                          (uint8_t)(register_count >> 8), (uint8_t)register_count};
    append_crc(request, 6);
    uint8_t response[HEAT_MODBUS_MAX_PDU + 2];
    size_t response_length = 0;
    esp_err_t err = exchange(master, request, sizeof(request), response, sizeof(response),
                             &response_length);
    if (err != ESP_OK) {
        return err;
    }
    if (response[2] != register_count * 2 || response_length < 5 + register_count * 2) {
        return ESP_ERR_INVALID_RESPONSE;
    }
    for (uint16_t index = 0; index < register_count; ++index) {
        registers[index] = ((uint16_t)response[3 + index * 2] << 8) | response[4 + index * 2];
    }
    return ESP_OK;
}

esp_err_t heat_modbus_write_single_register(heat_modbus_master_t *master,
                                             uint8_t slave_address,
                                             uint16_t register_address,
                                             uint16_t value) {
    if (master == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t request[8] = {slave_address, 0x06,
                          (uint8_t)(register_address >> 8), (uint8_t)register_address,
                          (uint8_t)(value >> 8), (uint8_t)value};
    append_crc(request, 6);
    uint8_t response[8];
    size_t response_length = 0;
    return exchange(master, request, sizeof(request), response, sizeof(response), &response_length);
}

esp_err_t heat_modbus_write_multiple_registers(heat_modbus_master_t *master,
                                               uint8_t slave_address,
                                               uint16_t register_address,
                                               uint16_t register_count,
                                               const uint16_t *registers) {
    if (master == NULL || registers == NULL || register_count == 0 ||
        register_count > 123) {
        return ESP_ERR_INVALID_ARG;
    }
    size_t frame_length_without_crc = 7 + register_count * 2;
    uint8_t request[253];
    request[0] = slave_address;
    request[1] = 0x10;
    request[2] = (uint8_t)(register_address >> 8);
    request[3] = (uint8_t)register_address;
    request[4] = (uint8_t)(register_count >> 8);
    request[5] = (uint8_t)register_count;
    request[6] = (uint8_t)(register_count * 2);
    for (uint16_t index = 0; index < register_count; ++index) {
        request[7 + index * 2] = (uint8_t)(registers[index] >> 8);
        request[8 + index * 2] = (uint8_t)registers[index];
    }
    append_crc(request, frame_length_without_crc);
    uint8_t response[8];
    size_t response_length = 0;
    return exchange(master, request, frame_length_without_crc + 2, response, sizeof(response),
                    &response_length);
}
