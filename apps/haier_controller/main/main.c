#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_spiffs.h"
#include "nvs_flash.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board_28141.h"
#include "heat_ch422g.h"
#include "heat_observer.h"
#include "heat_web.h"

static const char *TAG = "haier_controller";

void app_main(void)
{
    i2c_master_bus_handle_t i2c_bus = NULL;
    heat_ch422g_t io_expander = {0};
    uint8_t input_state = 0;

    ESP_LOGI(TAG, "Haier controller starting on %s", BOARD_MODEL_NAME);
    ESP_LOGI(TAG, "Initial mode: passive RS485 observation");
    ESP_LOGI(TAG, "RS485 UART pins: TX=%d RX=%d, automatic direction",
             BOARD_RS485_TX, BOARD_RS485_RX);

    esp_err_t error = nvs_flash_init();
    if (error == ESP_ERR_NVS_NO_FREE_PAGES || error == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        error = nvs_flash_erase();
        if (error == ESP_OK) {
            error = nvs_flash_init();
        }
    }
    if (error == ESP_OK) {
        error = esp_netif_init();
    }
    if (error == ESP_OK) {
        error = esp_event_loop_create_default();
    }
    if (error == ESP_OK) {
        const esp_vfs_spiffs_conf_t storage_config = {
            .base_path = "/storage",
            .partition_label = "storage",
            .max_files = 8,
            .format_if_mount_failed = false,
        };
        error = esp_vfs_spiffs_register(&storage_config);
    }
    if (error == ESP_OK) {
        error = heat_web_start();
    }
    if (error != ESP_OK) {
        ESP_LOGW(TAG, "Web HMI unavailable: %s", esp_err_to_name(error));
    }

    const heat_observer_config_t observer_config = {
        .uart_port = UART_NUM_1,
        .tx_gpio = BOARD_RS485_TX,
        .rx_gpio = BOARD_RS485_RX,
        .baud_rate = 9600,
    };
    error = heat_observer_start(&observer_config);
    if (error != ESP_OK) {
        ESP_LOGW(TAG, "Passive RS485 observer unavailable: %s", esp_err_to_name(error));
    }

    const i2c_master_bus_config_t i2c_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = BOARD_IO_EXPANDER_SDA,
        .scl_io_num = BOARD_IO_EXPANDER_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    error = i2c_new_master_bus(&i2c_config, &i2c_bus);
    if (error == ESP_OK) {
        const heat_ch422g_config_t expander_config = {
            .bus = i2c_bus,
            .address = HEAT_CH422G_DEFAULT_ADDRESS,
            .scl_speed_hz = HEAT_CH422G_DEFAULT_SCL_SPEED_HZ,
        };
        error = heat_ch422g_init(&io_expander, &expander_config);
    }
    if (error == ESP_OK) {
        error = heat_ch422g_read_inputs(&io_expander, &input_state);
    }
    if (error == ESP_OK) {
        ESP_LOGI(TAG, "CH422G detected; terminal inputs=0x%02x", input_state);
    } else {
        ESP_LOGW(TAG, "CH422G read-only probe unavailable: %s", esp_err_to_name(error));
    }

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
