#include "heat_ch422g.h"

#include <string.h>

#include "esp_check.h"
#include "freertos/FreeRTOS.h"

static const char *TAG = "heat_ch422g";

esp_err_t heat_ch422g_init(heat_ch422g_t *expander,
                           const heat_ch422g_config_t *config)
{
    ESP_RETURN_ON_FALSE(expander != NULL, ESP_ERR_INVALID_ARG, TAG, "expander is null");
    ESP_RETURN_ON_FALSE(config != NULL, ESP_ERR_INVALID_ARG, TAG, "config is null");
    memset(expander, 0, sizeof(*expander));
    ESP_RETURN_ON_FALSE(config->bus != NULL, ESP_ERR_INVALID_ARG, TAG,
                        "I2C bus is null");
    expander->address = config->address == 0 ? HEAT_CH422G_DEFAULT_ADDRESS : config->address;
    expander->output_latch = 0;

    const i2c_device_config_t control_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = expander->address,
        .scl_speed_hz = config->scl_speed_hz == 0 ? HEAT_CH422G_DEFAULT_SCL_SPEED_HZ : config->scl_speed_hz,
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(config->bus, &control_config,
                                                  &expander->control_device),
                       TAG, "failed to add CH422G control device");

    const i2c_device_config_t output_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x38,
        .scl_speed_hz = config->scl_speed_hz == 0 ? HEAT_CH422G_DEFAULT_SCL_SPEED_HZ : config->scl_speed_hz,
    };
    return i2c_master_bus_add_device(config->bus, &output_config,
                                     &expander->output_device);
}

esp_err_t heat_ch422g_read_inputs(const heat_ch422g_t *expander,
                                  uint8_t *input_state)
{
    ESP_RETURN_ON_FALSE(expander != NULL,
                        ESP_ERR_INVALID_ARG, TAG, "expander is not initialized");
    ESP_RETURN_ON_FALSE(input_state != NULL, ESP_ERR_INVALID_ARG, TAG,
                        "input state is null");

    const uint8_t command = HEAT_CH422G_INPUT_COMMAND;
    return i2c_master_transmit_receive(expander->control_device,
                                       &command, 1, input_state, 1,
                                       1000);
}

esp_err_t heat_ch422g_prepare_output_mode(heat_ch422g_t *expander)
{
    ESP_RETURN_ON_FALSE(expander != NULL,
                        ESP_ERR_INVALID_ARG, TAG, "expander is not initialized");
    const uint8_t command = 0x01;
    return i2c_master_transmit(expander->control_device, &command, 1, 1000);
}

esp_err_t heat_ch422g_read_input(const heat_ch422g_t *expander,
                                 heat_ch422g_input_t input,
                                 bool *active)
{
    ESP_RETURN_ON_FALSE(input <= HEAT_CH422G_DI1, ESP_ERR_INVALID_ARG, TAG,
                        "invalid input channel");
    ESP_RETURN_ON_FALSE(active != NULL, ESP_ERR_INVALID_ARG, TAG,
                        "active state is null");

    uint8_t input_state = 0;
    ESP_RETURN_ON_ERROR(heat_ch422g_read_inputs(expander, &input_state), TAG,
                        "failed to read inputs");
    *active = (input_state & (1u << input)) != 0;
    return ESP_OK;
}

esp_err_t heat_ch422g_write_outputs(heat_ch422g_t *expander,
                                    uint8_t output_state)
{
    ESP_RETURN_ON_FALSE(expander != NULL,
                        ESP_ERR_INVALID_ARG, TAG, "expander is not initialized");

    const uint8_t command = output_state;
    esp_err_t error = i2c_master_transmit(expander->output_device, &command, 1, 1000);
    if (error == ESP_OK) {
        expander->output_latch = output_state;
    }
    return error;
}

esp_err_t heat_ch422g_write_output(heat_ch422g_t *expander,
                                   heat_ch422g_output_t output,
                                   bool active)
{
    ESP_RETURN_ON_FALSE(output <= HEAT_CH422G_DO1, ESP_ERR_INVALID_ARG, TAG,
                        "invalid output channel");

    uint8_t output_state = expander == NULL ? 0 : expander->output_latch;
    if (active) {
        output_state |= (uint8_t)(1u << output);
    } else {
        output_state &= (uint8_t)~(1u << output);
    }
    return heat_ch422g_write_outputs(expander, output_state);
}
