#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define HEAT_NODE_HMI 1
#define HEAT_NODE_PLANT 2
#define HEAT_CHANNEL_STABLE "stable"
#define HEAT_CHANNEL_BETA "beta"

#define HEAT_MAX_SENSORS 16
#define HEAT_MAX_ZONES 5
#define HEAT_MAX_ALARMS 16
#define HEAT_MAX_EVENTS 64
#define HEAT_MAX_RELAYS 8
#define HEAT_MAX_INPUTS 8
#define HEAT_DEFAULT_FEEDBACK_TIMEOUT_MS 15000u

#define HEAT_RS485_BAUD 9600u
#define HEAT_RS485_DATA_BITS 8u
#define HEAT_RS485_PARITY_NONE 1
#define HEAT_RS485_STOP_BITS 1u
#define HEAT_RS485_FLOW_CONTROL_NONE 1
/* Field-verified: some XY-MD02 units reply ~205-210ms after the request, so
 * 200ms clipped valid responses right at the edge; 400ms gives real margin. */
#define HEAT_RS485_TIMEOUT_MS 400u

#define HEAT_TAG "heat-common"

typedef enum {
    HEAT_SENSOR_TYPE_NONE = 0,
    HEAT_SENSOR_TYPE_FLOW,
    HEAT_SENSOR_TYPE_RETURN,
    HEAT_SENSOR_TYPE_OUTSIDE,
    HEAT_SENSOR_TYPE_ZONE_1,
    HEAT_SENSOR_TYPE_ZONE_2,
    HEAT_SENSOR_TYPE_ZONE_3,
    HEAT_SENSOR_TYPE_ZONE_4,
    HEAT_SENSOR_TYPE_ZONE_5,
    HEAT_SENSOR_TYPE_DHW,
    HEAT_SENSOR_TYPE_REMOTE
} heat_sensor_type_t;

typedef enum {
    HEAT_ZONE_1 = 0,
    HEAT_ZONE_2,
    HEAT_ZONE_3,
    HEAT_ZONE_4,
    HEAT_ZONE_5,
    HEAT_ZONE_UPSTAIRS = HEAT_ZONE_1,
    HEAT_ZONE_DOWNSTAIRS = HEAT_ZONE_2
} heat_zone_id_t;

typedef enum {
    HEAT_MODE_OFF = 0,
    HEAT_MODE_HEATING,
    HEAT_MODE_DHW,
    HEAT_MODE_HEATING_AND_DHW
} heat_operation_mode_t;

typedef enum {
    HEAT_SENSOR_SOURCE_NONE = 0,
    HEAT_SENSOR_SOURCE_HMI_RS485,
    HEAT_SENSOR_SOURCE_HMI_DS18B20,
    HEAT_SENSOR_SOURCE_PLANT_RS485,
    HEAT_SENSOR_SOURCE_PLANT_DS18B20
} heat_sensor_source_t;

typedef enum {
    HEAT_MSG_TYPE_PING = 0x10,
    HEAT_MSG_TYPE_STATUS = 0x11,
    HEAT_MSG_TYPE_SETPOINT = 0x12,
    HEAT_MSG_TYPE_SENSOR_MAP = 0x13,
    HEAT_MSG_TYPE_CONFIG = 0x14,
    HEAT_MSG_TYPE_ALARM = 0x15,
    HEAT_MSG_TYPE_OTA_CHECK = 0x16,
    HEAT_MSG_TYPE_OTA_RESULT = 0x17,
    HEAT_MSG_TYPE_ACK = 0x7E,
    HEAT_MSG_TYPE_NACK = 0x7F
} heat_msg_type_t;

typedef struct {
    uint8_t sensor_id[8];
    uint8_t node_id;
    uint8_t port_index;
    heat_sensor_type_t sensor_type;
    heat_sensor_source_t source;
    bool enabled;
} heat_sensor_assignment_t;

typedef struct {
    uint8_t zone_id;
    float target_temp_c;
    float current_temp_c;
    bool enabled;
    bool schedule_enabled;
} heat_zone_config_t;

typedef enum {
    HEAT_ERROR_FLAG_ZONE_1_FEEDBACK = (1u << 0),
    HEAT_ERROR_FLAG_ZONE_2_FEEDBACK = (1u << 1),
    HEAT_ERROR_FLAG_ZONE_3_FEEDBACK = (1u << 2),
    HEAT_ERROR_FLAG_ZONE_4_FEEDBACK = (1u << 3),
    HEAT_ERROR_FLAG_ZONE_5_FEEDBACK = (1u << 4),
    HEAT_ERROR_FLAG_DHW_FEEDBACK = (1u << 5),
    HEAT_ERROR_FLAG_BOILER_FEEDBACK = (1u << 6),
    HEAT_ERROR_FLAG_PUMP_FEEDBACK = (1u << 7)
} heat_error_flag_t;

typedef struct {
    bool enabled;
    bool active;
    bool alarm;
    uint32_t timeout_ms;
    int64_t deadline_us;
} heat_feedback_monitor_t;

typedef struct {
    uint8_t zone_id;
    float target_temp_c;
    float current_temp_c;
    float start_delta_temp_c;
    bool enabled;
    bool schedule_enabled;
    bool schedule_slot_enabled;
    bool heating_active;
    heat_feedback_monitor_t feedback;
} heat_zone_control_t;

typedef struct {
    float current_temp_c;
    float target_temp_c;
    float start_delta_temp_c;
    bool enabled;
    bool schedule_enabled;
    bool schedule_slot_enabled;
    bool production_active;
    bool priority_enabled;
    bool window_enabled;
    uint16_t current_minute;
    uint16_t window_start_minute;
    uint16_t window_end_minute;
    heat_feedback_monitor_t feedback;
} heat_dhw_control_t;

typedef struct {
    heat_operation_mode_t operation_mode;
    heat_zone_control_t zones[HEAT_MAX_ZONES];
    heat_dhw_control_t dhw;
    heat_feedback_monitor_t boiler_feedback;
    heat_feedback_monitor_t pump_feedback;
} heat_control_state_t;

typedef struct {
    bool enabled;
    float outdoor_warm_c;
    float outdoor_cold_c;
    float flow_warm_c;
    float flow_cold_c;
} heat_weather_curve_config_t;

typedef struct {
    uint8_t node_id;
    uint8_t node_type;
    uint32_t uptime_ms;
    float zone_current_temp_c[HEAT_MAX_ZONES];
    float zone_setpoint_temp_c[HEAT_MAX_ZONES];
    bool zone_enabled[HEAT_MAX_ZONES];
    bool zone_heat_request[HEAT_MAX_ZONES];
    bool zone_feedback_alarm[HEAT_MAX_ZONES];
    float dhw_current_temp_c;
    float dhw_setpoint_temp_c;
    bool dhw_enabled;
    bool dhw_heat_request;
    bool dhw_feedback_alarm;
    float flow_temp_c;
    float return_temp_c;
    float outside_temp_c;
    bool boiler_demand;
    bool pump_enable;
    bool boiler_feedback_alarm;
    bool pump_feedback_alarm;
    bool valve_upstairs;
    bool valve_downstairs;
    bool valve_dhw;
    bool relay_active[HEAT_MAX_RELAYS];
    bool input_active[HEAT_MAX_INPUTS];
    uint32_t error_flags;
    bool weather_curve_active;
    bool dhw_schedule_active;
    bool zone_schedule_active[HEAT_MAX_ZONES];
    bool communications_ok;
    bool watchdog_ok;
} heat_status_t;

typedef struct {
    uint8_t version_major;
    uint8_t version_minor;
    uint8_t version_patch;
    char channel[8];
    uint32_t build_epoch;
    char firmware_name[32];
} heat_ota_info_t;

typedef struct {
    uint8_t source_node_id;
    uint8_t target_node_id;
    uint8_t message_type;
    uint16_t payload_length;
    uint8_t payload[128];
} heat_rs485_frame_t;

bool heat_common_init(void);
bool heat_dhw_heating_required(heat_operation_mode_t mode, float current_temp_c,
                               float setpoint_temp_c, float delta_setpoint_c);
bool heat_dhw_production_required(heat_operation_mode_t mode, float current_temp_c,
                                  float start_temp_c, float target_temp_c,
                                  bool production_active, bool window_enabled,
                                  uint16_t current_minute,
                                  uint16_t window_start_minute, uint16_t window_end_minute);
bool heat_dhw_priority_blocks_heating(bool priority_enabled, bool dhw_heating_required);
bool heat_zone_heating_required(heat_operation_mode_t mode,
                                const heat_zone_config_t *zone);
float heat_weather_curve_flow_target(const heat_weather_curve_config_t *curve,
                                     float outdoor_temp_c);
void heat_control_state_init(heat_control_state_t *state);
bool heat_zone_control_demand_required(heat_operation_mode_t mode,
                                       const heat_zone_control_t *zone);
bool heat_dhw_control_demand_required(heat_operation_mode_t mode,
                                      const heat_dhw_control_t *dhw);
void heat_apply_control_state(const heat_control_state_t *control, heat_status_t *status);
void heat_feedback_monitor_update(heat_feedback_monitor_t *monitor, bool commanded_on,
                                  bool input_active, int64_t now_us);
void heat_common_set_node_id(uint8_t node_id);
uint8_t heat_common_get_node_id(void);
void heat_common_set_ota_channel(const char *channel);
const char *heat_common_get_ota_channel(void);
void heat_common_fill_status(heat_status_t *status);
int heat_common_encode_frame(const heat_rs485_frame_t *frame, uint8_t *out, size_t out_len);
int heat_common_decode_frame(const uint8_t *in, size_t in_len, heat_rs485_frame_t *frame);
