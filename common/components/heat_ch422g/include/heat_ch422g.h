#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HEAT_CH422G_DEFAULT_ADDRESS 0x24u
#define HEAT_CH422G_DEFAULT_SCL_SPEED_HZ 400000u
#define HEAT_CH422G_INPUT_COMMAND 0x4Du
#define HEAT_CH422G_OUTPUT_COMMAND 0x48u

typedef enum {
    HEAT_CH422G_DI0 = 0,
    HEAT_CH422G_DI1 = 1,
} heat_ch422g_input_t;

typedef enum {
    HEAT_CH422G_DO0 = 0,
    HEAT_CH422G_DO1 = 1,
} heat_ch422g_output_t;

typedef struct {
    i2c_master_bus_handle_t bus;
    uint8_t address;
    uint32_t scl_speed_hz;
} heat_ch422g_config_t;

typedef struct {
    i2c_master_dev_handle_t device;
    uint8_t output_latch;
} heat_ch422g_t;

esp_err_t heat_ch422g_init(heat_ch422g_t *expander,
                           const heat_ch422g_config_t *config);
esp_err_t heat_ch422g_read_inputs(const heat_ch422g_t *expander,
                                  uint8_t *input_state);
esp_err_t heat_ch422g_read_input(const heat_ch422g_t *expander,
                                 heat_ch422g_input_t input,
                                 bool *active);
esp_err_t heat_ch422g_write_outputs(heat_ch422g_t *expander,
                                    uint8_t output_state);
esp_err_t heat_ch422g_write_output(heat_ch422g_t *expander,
                                   heat_ch422g_output_t output,
                                   bool active);

#ifdef __cplusplus
}
#endif
