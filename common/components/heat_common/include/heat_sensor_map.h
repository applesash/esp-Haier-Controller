#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "heat_common.h"

#define HEAT_SENSOR_MAP_COUNT 16

typedef struct {
    uint8_t index;
    uint8_t node_id;
    uint8_t sensor_id[8];
    heat_sensor_type_t sensor_type;
    heat_sensor_source_t source;
    bool active;
} heat_sensor_map_entry_t;

typedef struct {
    uint8_t count;
    heat_sensor_map_entry_t entries[HEAT_SENSOR_MAP_COUNT];
} heat_sensor_map_t;

void heat_sensor_map_init(heat_sensor_map_t *map);
bool heat_sensor_map_set(heat_sensor_map_t *map, const heat_sensor_assignment_t *assignment);
bool heat_sensor_map_get(const heat_sensor_map_t *map, uint8_t index, heat_sensor_map_entry_t *entry);
bool heat_sensor_map_remove(heat_sensor_map_t *map, uint8_t index);
