#include "heat_common.h"
#include "heat_modbus_protocol.h"

#include <stdio.h>
#include <string.h>

static uint8_t g_node_id = 1;
static char g_ota_channel[8] = HEAT_CHANNEL_STABLE;

static uint32_t zone_feedback_error_flag(uint8_t zone_index) {
    static const uint32_t flags[HEAT_MAX_ZONES] = {
        HEAT_ERROR_FLAG_ZONE_1_FEEDBACK,
        HEAT_ERROR_FLAG_ZONE_2_FEEDBACK,
        HEAT_ERROR_FLAG_ZONE_3_FEEDBACK,
        HEAT_ERROR_FLAG_ZONE_4_FEEDBACK,
        HEAT_ERROR_FLAG_ZONE_5_FEEDBACK,
    };
    return zone_index < HEAT_MAX_ZONES ? flags[zone_index] : 0u;
}

static bool heating_mode_enabled(heat_operation_mode_t mode) {
    return mode == HEAT_MODE_HEATING || mode == HEAT_MODE_HEATING_AND_DHW;
}

static bool dhw_mode_enabled(heat_operation_mode_t mode) {
    return mode == HEAT_MODE_DHW || mode == HEAT_MODE_HEATING_AND_DHW;
}

static float demand_start_threshold(float target_temp_c, float start_delta_temp_c) {
    return target_temp_c - start_delta_temp_c;
}

void heat_modbus_command_encode(const heat_modbus_command_t *command,
                                uint16_t registers[HEAT_MODBUS_COMMAND_REGISTER_COUNT]) {
    if (command == NULL || registers == NULL) {
        return;
    }
    registers[0] = command->sequence;
    registers[1] = (uint16_t)command->operation_mode;
    registers[2] = command->demand_bits;
    registers[3] = (command->sensor_mode_enabled ? HEAT_MODBUS_FLOW_SENSOR_MODE : 0u) |
                   (command->weather_curve_enabled ? HEAT_MODBUS_FLOW_WEATHER_CURVE : 0u);
    registers[4] = (uint16_t)command->flow_target_deci_c;
    registers[5] = command->feedback_enabled_bits;
    registers[6] = (uint16_t)command->flow_temp_deci_c;
    registers[7] = (uint16_t)command->return_temp_deci_c;
    registers[8] = (uint16_t)command->outdoor_temp_deci_c;
    registers[9] = (uint16_t)command->dhw_temp_deci_c;
    registers[10] = command->temperature_valid_bits;
}

bool heat_modbus_command_decode(const uint16_t registers[HEAT_MODBUS_COMMAND_REGISTER_COUNT],
                                heat_modbus_command_t *command) {
    if (registers == NULL || command == NULL ||
        registers[1] > (uint16_t)HEAT_MODE_HEATING_AND_DHW ||
        (registers[2] & (uint16_t)~0x003Fu) != 0u ||
        (registers[3] & (uint16_t)~0x0003u) != 0u ||
        (registers[5] & (uint16_t)~0x00FFu) != 0u ||
        (registers[10] & (uint16_t)~0x000Fu) != 0u) {
        return false;
    }
    command->sequence = registers[0];
    command->operation_mode = (heat_operation_mode_t)registers[1];
    command->demand_bits = registers[2];
    command->sensor_mode_enabled = (registers[3] & HEAT_MODBUS_FLOW_SENSOR_MODE) != 0;
    command->weather_curve_enabled =
        (registers[3] & HEAT_MODBUS_FLOW_WEATHER_CURVE) != 0;
    command->flow_target_deci_c = (int16_t)registers[4];
    command->feedback_enabled_bits = registers[5];
    command->flow_temp_deci_c = (int16_t)registers[6];
    command->return_temp_deci_c = (int16_t)registers[7];
    command->outdoor_temp_deci_c = (int16_t)registers[8];
    command->dhw_temp_deci_c = (int16_t)registers[9];
    command->temperature_valid_bits = registers[10];
    if (command->flow_temp_deci_c < -550 || command->flow_temp_deci_c > 1250 ||
        command->return_temp_deci_c < -550 || command->return_temp_deci_c > 1250 ||
        command->outdoor_temp_deci_c < -550 || command->outdoor_temp_deci_c > 1250 ||
        command->dhw_temp_deci_c < -550 || command->dhw_temp_deci_c > 1250) {
        return false;
    }
    return true;
}

void heat_modbus_status_encode(const heat_modbus_status_t *status,
                               uint16_t registers[HEAT_MODBUS_STATUS_REGISTER_COUNT]) {
    if (status == NULL || registers == NULL) {
        return;
    }
    registers[0] = status->state_bits;
    registers[1] = status->relay_bits;
    registers[2] = status->input_bits;
    registers[3] = (uint16_t)(status->alarm_bits & 0xFFFFu);
    registers[4] = (uint16_t)(status->alarm_bits >> 16);
    registers[5] = status->accepted_demand_bits;
    registers[6] = (uint16_t)status->flow_temp_deci_c;
    registers[7] = (uint16_t)status->return_temp_deci_c;
    registers[8] = (uint16_t)status->outdoor_temp_deci_c;
    registers[9] = (uint16_t)status->active_flow_target_deci_c;
    registers[10] = status->accepted_sequence;
}

void heat_modbus_status_decode(const uint16_t registers[HEAT_MODBUS_STATUS_REGISTER_COUNT],
                               heat_modbus_status_t *status) {
    if (registers == NULL || status == NULL) {
        return;
    }
    status->state_bits = registers[0];
    status->relay_bits = registers[1];
    status->input_bits = registers[2];
    status->alarm_bits = (uint32_t)registers[3] | ((uint32_t)registers[4] << 16);
    status->accepted_demand_bits = registers[5];
    status->flow_temp_deci_c = (int16_t)registers[6];
    status->return_temp_deci_c = (int16_t)registers[7];
    status->outdoor_temp_deci_c = (int16_t)registers[8];
    status->active_flow_target_deci_c = (int16_t)registers[9];
    status->accepted_sequence = registers[10];
}

bool heat_common_init(void) {
    return true;
}

bool heat_dhw_heating_required(heat_operation_mode_t mode, float current_temp_c,
                               float setpoint_temp_c, float delta_setpoint_c) {
    if (mode != HEAT_MODE_DHW && mode != HEAT_MODE_HEATING_AND_DHW) {
        return false;
    }
    const float start_temp_c = setpoint_temp_c + delta_setpoint_c;
    return current_temp_c <= start_temp_c;
}

bool heat_dhw_production_required(heat_operation_mode_t mode, float current_temp_c,
                                  float start_temp_c, float target_temp_c,
                                  bool production_active, bool window_enabled,
                                  uint16_t current_minute,
                                  uint16_t window_start_minute, uint16_t window_end_minute) {
    if (mode != HEAT_MODE_DHW && mode != HEAT_MODE_HEATING_AND_DHW) {
        return false;
    }
    if (window_enabled && (current_minute < window_start_minute ||
                           current_minute >= window_end_minute)) {
        return false;
    }
    return production_active ? current_temp_c < target_temp_c : current_temp_c <= start_temp_c;
}

bool heat_dhw_priority_blocks_heating(bool priority_enabled, bool dhw_heating_required) {
    return priority_enabled && dhw_heating_required;
}

bool heat_zone_heating_required(heat_operation_mode_t mode,
                               const heat_zone_config_t *zone) {
    if (zone == NULL || !zone->enabled ||
        (mode != HEAT_MODE_HEATING && mode != HEAT_MODE_HEATING_AND_DHW)) {
        return false;
    }
    return zone->current_temp_c < zone->target_temp_c;
}

float heat_weather_curve_flow_target(const heat_weather_curve_config_t *curve,
                                     float outdoor_temp_c) {
    if (curve == NULL || !curve->enabled) {
        return 0.0f;
    }
    if (outdoor_temp_c <= curve->outdoor_cold_c) {
        return curve->flow_cold_c;
    }
    if (outdoor_temp_c >= curve->outdoor_warm_c) {
        return curve->flow_warm_c;
    }
    float span = curve->outdoor_warm_c - curve->outdoor_cold_c;
    if (span <= 0.0f) {
        return curve->flow_warm_c;
    }
    float fraction = (outdoor_temp_c - curve->outdoor_cold_c) / span;
    return curve->flow_cold_c + fraction * (curve->flow_warm_c - curve->flow_cold_c);
}

void heat_control_state_init(heat_control_state_t *state) {
    if (state == NULL) {
        return;
    }

    memset(state, 0, sizeof(*state));
    state->operation_mode = HEAT_MODE_HEATING_AND_DHW;
    for (uint8_t zone = 0; zone < HEAT_MAX_ZONES; ++zone) {
        state->zones[zone].zone_id = zone;
        state->zones[zone].enabled = true;
        state->zones[zone].schedule_enabled = true;
        state->zones[zone].schedule_slot_enabled = true;
        state->zones[zone].start_delta_temp_c = 1.0f;
        state->zones[zone].feedback.enabled = true;
        state->zones[zone].feedback.timeout_ms = HEAT_DEFAULT_FEEDBACK_TIMEOUT_MS;
        state->zones[zone].target_temp_c = 19.0f;
        state->zones[zone].current_temp_c = 20.0f;
    }
    state->zones[0].current_temp_c = 20.8f;
    state->zones[1].current_temp_c = 21.4f;

    state->dhw.enabled = true;
    state->dhw.schedule_enabled = true;
    state->dhw.schedule_slot_enabled = true;
    state->dhw.priority_enabled = true;
    state->dhw.window_enabled = true;
    state->dhw.window_start_minute = 5u * 60u;
    state->dhw.window_end_minute = 22u * 60u;
    state->dhw.current_minute = 12u * 60u;
    state->dhw.start_delta_temp_c = 5.0f;
    state->dhw.target_temp_c = 55.0f;
    state->dhw.current_temp_c = 48.6f;
    state->dhw.feedback.enabled = true;
    state->dhw.feedback.timeout_ms = HEAT_DEFAULT_FEEDBACK_TIMEOUT_MS;

    state->boiler_feedback.enabled = true;
    state->boiler_feedback.timeout_ms = HEAT_DEFAULT_FEEDBACK_TIMEOUT_MS;
    state->pump_feedback.enabled = true;
    state->pump_feedback.timeout_ms = HEAT_DEFAULT_FEEDBACK_TIMEOUT_MS;
}

/* Shared hysteresis/feedback/schedule evaluation for zone and DHW demand.
 * window_ok is always true for zones, which have no production window. */
typedef struct {
    float current_temp_c;
    float target_temp_c;
    float start_delta_temp_c;
    bool enabled;
    bool schedule_enabled;
    bool schedule_slot_enabled;
    bool heating_active;
    heat_feedback_monitor_t feedback;
} heat_demand_params_t;

static bool heat_demand_required_generic(bool mode_enabled, const heat_demand_params_t *params,
                                         bool window_ok) {
    if (params == NULL || !params->enabled || !mode_enabled || !window_ok) {
        return false;
    }
    if (params->feedback.enabled && params->feedback.alarm) {
        return false;
    }
    if (params->schedule_enabled && !params->schedule_slot_enabled) {
        return false;
    }

    const float start_threshold =
        demand_start_threshold(params->target_temp_c, params->start_delta_temp_c);
    if (params->heating_active) {
        return params->current_temp_c < params->target_temp_c;
    }
    return params->current_temp_c <= start_threshold;
}

bool heat_zone_control_demand_required(heat_operation_mode_t mode,
                                       const heat_zone_control_t *zone) {
    if (zone == NULL) {
        return false;
    }
    const heat_demand_params_t params = {
        .current_temp_c = zone->current_temp_c,
        .target_temp_c = zone->target_temp_c,
        .start_delta_temp_c = zone->start_delta_temp_c,
        .enabled = zone->enabled,
        .schedule_enabled = zone->schedule_enabled,
        .schedule_slot_enabled = zone->schedule_slot_enabled,
        .heating_active = zone->heating_active,
        .feedback = zone->feedback,
    };
    return heat_demand_required_generic(heating_mode_enabled(mode), &params, true);
}

bool heat_dhw_control_demand_required(heat_operation_mode_t mode,
                                      const heat_dhw_control_t *dhw) {
    if (dhw == NULL) {
        return false;
    }
    const bool window_ok = !dhw->window_enabled ||
        (dhw->current_minute >= dhw->window_start_minute &&
         dhw->current_minute < dhw->window_end_minute);
    const heat_demand_params_t params = {
        .current_temp_c = dhw->current_temp_c,
        .target_temp_c = dhw->target_temp_c,
        .start_delta_temp_c = dhw->start_delta_temp_c,
        .enabled = dhw->enabled,
        .schedule_enabled = dhw->schedule_enabled,
        .schedule_slot_enabled = dhw->schedule_slot_enabled,
        .heating_active = dhw->production_active,
        .feedback = dhw->feedback,
    };
    return heat_demand_required_generic(dhw_mode_enabled(mode), &params, window_ok);
}

void heat_apply_control_state(const heat_control_state_t *control, heat_status_t *status) {
    if (control == NULL || status == NULL) {
        return;
    }

    bool zone_output_active[HEAT_MAX_ZONES] = {false};
    bool any_zone_output_active = false;
    bool any_zone_heat_request = false;

    status->error_flags = 0u;
    for (uint8_t zone = 0; zone < HEAT_MAX_ZONES; ++zone) {
        const heat_zone_control_t *zone_control = &control->zones[zone];
        const bool zone_request =
            heat_zone_control_demand_required(control->operation_mode, zone_control);

        status->zone_current_temp_c[zone] = zone_control->current_temp_c;
        status->zone_setpoint_temp_c[zone] = zone_control->target_temp_c;
        status->zone_enabled[zone] = zone_control->enabled;
        status->zone_heat_request[zone] = zone_request;
        status->zone_feedback_alarm[zone] =
            zone_control->enabled && zone_control->feedback.enabled && zone_control->feedback.alarm;
        status->zone_schedule_active[zone] =
            zone_control->enabled && zone_control->schedule_enabled &&
            zone_control->schedule_slot_enabled;
        if (status->zone_feedback_alarm[zone]) {
            status->error_flags |= zone_feedback_error_flag(zone);
        }
        if (zone_request) {
            any_zone_heat_request = true;
        }
    }

    status->dhw_current_temp_c = control->dhw.current_temp_c;
    status->dhw_setpoint_temp_c = control->dhw.target_temp_c;
    status->dhw_enabled = control->dhw.enabled;
    status->dhw_feedback_alarm =
        control->dhw.enabled && control->dhw.feedback.enabled && control->dhw.feedback.alarm;
    status->dhw_heat_request =
        heat_dhw_control_demand_required(control->operation_mode, &control->dhw);
    status->dhw_schedule_active =
        control->dhw.enabled && control->dhw.schedule_enabled &&
        control->dhw.schedule_slot_enabled;
    if (status->dhw_feedback_alarm) {
        status->error_flags |= HEAT_ERROR_FLAG_DHW_FEEDBACK;
    }

    status->boiler_feedback_alarm =
        control->boiler_feedback.enabled && control->boiler_feedback.alarm;
    status->pump_feedback_alarm =
        control->pump_feedback.enabled && control->pump_feedback.alarm;
    if (status->boiler_feedback_alarm) {
        status->error_flags |= HEAT_ERROR_FLAG_BOILER_FEEDBACK;
    }
    if (status->pump_feedback_alarm) {
        status->error_flags |= HEAT_ERROR_FLAG_PUMP_FEEDBACK;
    }

    const bool heating_blocked_by_dhw_priority =
        control->dhw.priority_enabled && status->dhw_heat_request;
    for (uint8_t zone = 0; zone < HEAT_MAX_ZONES; ++zone) {
        zone_output_active[zone] =
            status->zone_enabled[zone] && status->zone_heat_request[zone] &&
            !heating_blocked_by_dhw_priority;
        any_zone_output_active = any_zone_output_active || zone_output_active[zone];
    }

    status->valve_upstairs = zone_output_active[HEAT_ZONE_1];
    status->valve_downstairs = zone_output_active[HEAT_ZONE_2];
    status->valve_dhw = status->dhw_heat_request;
    status->pump_enable = status->dhw_heat_request || any_zone_output_active;
    status->boiler_demand = status->pump_enable &&
                            (status->dhw_heat_request || any_zone_output_active ||
                             any_zone_heat_request);

    for (uint8_t relay = 0; relay < HEAT_MAX_RELAYS; ++relay) {
        status->relay_active[relay] = false;
    }
    status->relay_active[0] = status->boiler_demand;
    if (HEAT_MAX_RELAYS > 1) {
        status->relay_active[1] = status->pump_enable;
    }
    if (HEAT_MAX_RELAYS > 2) {
        status->relay_active[2] = status->valve_dhw;
    }
    for (uint8_t zone = 0; zone < HEAT_MAX_ZONES && (zone + 3u) < HEAT_MAX_RELAYS; ++zone) {
        status->relay_active[zone + 3u] = zone_output_active[zone];
    }

    for (uint8_t input = 0; input < HEAT_MAX_INPUTS; ++input) {
        status->input_active[input] = false;
    }
    status->input_active[0] = control->boiler_feedback.active;
    if (HEAT_MAX_INPUTS > 1) {
        status->input_active[1] = control->pump_feedback.active;
    }
    if (HEAT_MAX_INPUTS > 2) {
        status->input_active[2] = control->dhw.feedback.active;
    }
    for (uint8_t zone = 0; zone < HEAT_MAX_ZONES && (zone + 3u) < HEAT_MAX_INPUTS; ++zone) {
        status->input_active[zone + 3u] = control->zones[zone].feedback.active;
    }
}

void heat_feedback_monitor_update(heat_feedback_monitor_t *monitor, bool commanded_on,
                                  bool input_active, int64_t now_us) {
    if (monitor == NULL) {
        return;
    }
    if (!monitor->enabled) {
        /* No feedback point wired: the commanded relay state is the running state. */
        monitor->active = commanded_on;
        monitor->alarm = false;
        monitor->deadline_us = 0;
        return;
    }
    if (!commanded_on) {
        monitor->active = false;
        monitor->alarm = false;
        monitor->deadline_us = 0;
        return;
    }
    if (input_active) {
        monitor->active = true;
        monitor->alarm = false;
        monitor->deadline_us = 0;
        return;
    }
    monitor->active = false;
    if (monitor->deadline_us == 0) {
        const uint32_t timeout_ms =
            monitor->timeout_ms != 0 ? monitor->timeout_ms : HEAT_DEFAULT_FEEDBACK_TIMEOUT_MS;
        monitor->deadline_us = now_us + (int64_t)timeout_ms * 1000;
    } else if (now_us >= monitor->deadline_us) {
        monitor->alarm = true;
    }
}

void heat_common_set_node_id(uint8_t node_id) {
    g_node_id = node_id;
}

uint8_t heat_common_get_node_id(void) {
    return g_node_id;
}

void heat_common_set_ota_channel(const char *channel) {
    if (channel == NULL) {
        memcpy(g_ota_channel, HEAT_CHANNEL_STABLE, sizeof(HEAT_CHANNEL_STABLE));
        return;
    }
    snprintf(g_ota_channel, sizeof(g_ota_channel), "%s", channel);
}

const char *heat_common_get_ota_channel(void) {
    return g_ota_channel;
}

void heat_common_fill_status(heat_status_t *status) {
    if (status == NULL) {
        return;
    }
    memset(status, 0, sizeof(*status));
    status->node_id = g_node_id;
    status->node_type = g_node_id == HEAT_NODE_PLANT ? HEAT_NODE_PLANT : HEAT_NODE_HMI;
    status->uptime_ms = 0;
    for (size_t zone = 0; zone < HEAT_MAX_ZONES; ++zone) {
        status->zone_current_temp_c[zone] = 0.0f;
        status->zone_setpoint_temp_c[zone] = 0.0f;
        status->zone_enabled[zone] = false;
        status->zone_heat_request[zone] = false;
        status->zone_feedback_alarm[zone] = false;
    }
    status->dhw_current_temp_c = 0.0f;
    status->dhw_setpoint_temp_c = 0.0f;
    status->dhw_enabled = false;
    status->dhw_heat_request = false;
    status->dhw_feedback_alarm = false;
    status->flow_temp_c = 0.0f;
    status->return_temp_c = 0.0f;
    status->outside_temp_c = 0.0f;
    status->boiler_demand = false;
    status->pump_enable = false;
    status->boiler_feedback_alarm = false;
    status->pump_feedback_alarm = false;
    status->valve_upstairs = false;
    status->valve_downstairs = false;
    status->valve_dhw = false;
    for (size_t relay = 0; relay < HEAT_MAX_RELAYS; ++relay) {
        status->relay_active[relay] = false;
    }
    for (size_t input = 0; input < HEAT_MAX_INPUTS; ++input) {
        status->input_active[input] = false;
    }
    for (size_t zone = 0; zone < HEAT_MAX_ZONES; ++zone) {
        status->zone_schedule_active[zone] = false;
    }
    status->error_flags = 0;
    status->weather_curve_active = false;
    status->dhw_schedule_active = false;
    status->communications_ok = true;
    status->watchdog_ok = true;
}

int heat_common_encode_frame(const heat_rs485_frame_t *frame, uint8_t *out, size_t out_len) {
    if (frame == NULL || out == NULL || out_len < 8u + (size_t)frame->payload_length) {
        return -1;
    }

    size_t offset = 0;
    out[offset++] = 0xAA;
    out[offset++] = frame->source_node_id;
    out[offset++] = frame->target_node_id;
    out[offset++] = frame->message_type;
    out[offset++] = (uint8_t)((frame->payload_length >> 8) & 0xFF);
    out[offset++] = (uint8_t)(frame->payload_length & 0xFF);
    if (frame->payload_length > 0) {
        memcpy(&out[offset], frame->payload, frame->payload_length);
        offset += frame->payload_length;
    }
    out[offset++] = 0x55;
    return (int)offset;
}

int heat_common_decode_frame(const uint8_t *in, size_t in_len, heat_rs485_frame_t *frame) {
    if (in == NULL || frame == NULL || in_len < 8) {
        return -1;
    }

    size_t offset = 0;
    if (in[offset++] != 0xAA) {
        return -1;
    }
    frame->source_node_id = in[offset++];
    frame->target_node_id = in[offset++];
    frame->message_type = in[offset++];
    frame->payload_length = (uint16_t)((uint16_t)in[offset++] << 8);
    frame->payload_length |= in[offset++];
    if (frame->payload_length > sizeof(frame->payload)) {
        return -1;
    }
    if (in_len < 8u + (size_t)frame->payload_length) {
        return -1;
    }
    if (frame->payload_length > 0) {
        memcpy(frame->payload, &in[offset], frame->payload_length);
        offset += frame->payload_length;
    }
    if (in[offset] != 0x55) {
        return -1;
    }
    return (int)(offset + 1);
}
