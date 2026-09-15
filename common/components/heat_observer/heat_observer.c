#include "heat_observer.h"

#include <stdio.h>

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "heat_observer";
static uart_port_t s_uart_port;
static TaskHandle_t s_observer_task;

static void observer_task(void *context)
{
    (void)context;
    uint8_t buffer[256];
    while (true) {
        int count = uart_read_bytes(s_uart_port, buffer, sizeof(buffer),
                                    pdMS_TO_TICKS(100));
        if (count <= 0) {
            continue;
        }
        char line[3 * 64 + 1];
        size_t offset = 0;
        int displayed = count > 64 ? 64 : count;
        for (int index = 0; index < displayed && offset + 3 < sizeof(line); ++index) {
            offset += (size_t)snprintf(line + offset, sizeof(line) - offset,
                                       "%02x ", buffer[index]);
        }
        line[offset] = '\0';
        ESP_LOGI(TAG, "RX %d bytes: %s%s", count, line,
                 count > displayed ? "..." : "");
    }
}

esp_err_t heat_observer_start(const heat_observer_config_t *config)
{
    ESP_RETURN_ON_FALSE(config != NULL, ESP_ERR_INVALID_ARG, TAG, "config is null");
    ESP_RETURN_ON_FALSE(config->tx_gpio >= 0 && config->rx_gpio >= 0,
                        ESP_ERR_INVALID_ARG, TAG, "invalid UART pins");
    ESP_RETURN_ON_FALSE(config->baud_rate > 0, ESP_ERR_INVALID_ARG, TAG,
                        "invalid baud rate");

    const uart_config_t uart_config = {
        .baud_rate = (int)config->baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_RETURN_ON_ERROR(uart_driver_install(config->uart_port, 2048, 0, 0, NULL, 0),
                        TAG, "UART install failed");
    ESP_RETURN_ON_ERROR(uart_param_config(config->uart_port, &uart_config), TAG,
                        "UART config failed");
    ESP_RETURN_ON_ERROR(uart_set_pin(config->uart_port, config->tx_gpio,
                                     config->rx_gpio, UART_PIN_NO_CHANGE,
                                     UART_PIN_NO_CHANGE), TAG,
                        "UART pin setup failed");
    s_uart_port = config->uart_port;
    BaseType_t task_created = xTaskCreate(observer_task, "rs485_observer", 3072,
                                          NULL, 5, &s_observer_task);
    return task_created == pdPASS ? ESP_OK : ESP_ERR_NO_MEM;
}

esp_err_t heat_observer_stop(void)
{
    if (s_observer_task != NULL) {
        vTaskDelete(s_observer_task);
        s_observer_task = NULL;
    }
    return uart_driver_delete(s_uart_port);
}
