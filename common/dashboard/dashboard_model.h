#pragma once

#include <stdbool.h>
#include <stdint.h>

#define HEAT_DASHBOARD_WIDTH 800u
#define HEAT_DASHBOARD_HEIGHT 480u
#define HEAT_DASHBOARD_COLUMNS 3u
#define HEAT_DASHBOARD_TILE_COUNT 9u

typedef enum {
    HEAT_DASHBOARD_TILE_OUTDOOR = 0,
    HEAT_DASHBOARD_TILE_UPSTAIRS,
    HEAT_DASHBOARD_TILE_DOWNSTAIRS,
    HEAT_DASHBOARD_TILE_DHW,
    HEAT_DASHBOARD_TILE_FLOW,
    HEAT_DASHBOARD_TILE_RETURN,
    HEAT_DASHBOARD_TILE_FLOW_RATE,
    HEAT_DASHBOARD_TILE_THERMAL_OUTPUT,
    HEAT_DASHBOARD_TILE_THERMAL_INPUT,
} heat_dashboard_tile_id_t;

typedef enum {
    HEAT_DASHBOARD_SENSOR_NOT_CONFIGURED = 0,
    HEAT_DASHBOARD_SENSOR_AVAILABLE,
    HEAT_DASHBOARD_SENSOR_COMMS_ERROR,
} heat_dashboard_sensor_status_t;

typedef struct {
    heat_dashboard_tile_id_t id;
    heat_dashboard_sensor_status_t status;
    float value;
    float target;
    float humidity_percent;
    uint32_t age_seconds;
} heat_dashboard_reading_t;
