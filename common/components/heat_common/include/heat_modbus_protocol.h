#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "heat_common.h"

#define HEAT_MODBUS_PLANT_ADDRESS 10u

#define HEAT_MODBUS_COMMAND_REGISTER 0u
#define HEAT_MODBUS_COMMAND_REGISTER_COUNT 11u
#define HEAT_MODBUS_STATUS_REGISTER 0u
#define HEAT_MODBUS_STATUS_REGISTER_COUNT 11u

#define HEAT_MODBUS_DEMAND_DHW (1u << 0)
#define HEAT_MODBUS_DEMAND_ZONE_1 (1u << 1)
#define HEAT_MODBUS_DEMAND_ZONE_2 (1u << 2)
#define HEAT_MODBUS_DEMAND_ZONE_3 (1u << 3)
#define HEAT_MODBUS_DEMAND_ZONE_4 (1u << 4)
#define HEAT_MODBUS_DEMAND_ZONE_5 (1u << 5)

#define HEAT_MODBUS_FLOW_SENSOR_MODE (1u << 0)
#define HEAT_MODBUS_FLOW_WEATHER_CURVE (1u << 1)

#define HEAT_MODBUS_TEMP_FLOW_VALID (1u << 0)
#define HEAT_MODBUS_TEMP_RETURN_VALID (1u << 1)
#define HEAT_MODBUS_TEMP_OUTDOOR_VALID (1u << 2)
#define HEAT_MODBUS_TEMP_DHW_VALID (1u << 3)

typedef struct {
    uint16_t sequence;
    heat_operation_mode_t operation_mode;
    uint16_t demand_bits;
    bool sensor_mode_enabled;
    bool weather_curve_enabled;
    int16_t flow_target_deci_c;
    uint16_t feedback_enabled_bits;
    int16_t flow_temp_deci_c;
    int16_t return_temp_deci_c;
    int16_t outdoor_temp_deci_c;
    int16_t dhw_temp_deci_c;
    uint16_t temperature_valid_bits;
} heat_modbus_command_t;

typedef struct {
    uint16_t state_bits;
    uint16_t relay_bits;
    uint16_t input_bits;
    uint32_t alarm_bits;
    uint16_t accepted_demand_bits;
    int16_t flow_temp_deci_c;
    int16_t return_temp_deci_c;
    int16_t outdoor_temp_deci_c;
    int16_t active_flow_target_deci_c;
    uint16_t accepted_sequence;
} heat_modbus_status_t;

void heat_modbus_command_encode(const heat_modbus_command_t *command,
                                uint16_t registers[HEAT_MODBUS_COMMAND_REGISTER_COUNT]);
bool heat_modbus_command_decode(const uint16_t registers[HEAT_MODBUS_COMMAND_REGISTER_COUNT],
                                heat_modbus_command_t *command);
void heat_modbus_status_encode(const heat_modbus_status_t *status,
                               uint16_t registers[HEAT_MODBUS_STATUS_REGISTER_COUNT]);
void heat_modbus_status_decode(const uint16_t registers[HEAT_MODBUS_STATUS_REGISTER_COUNT],
                               heat_modbus_status_t *status);