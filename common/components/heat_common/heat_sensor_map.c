#include "heat_sensor_map.h"

#include <string.h>

void heat_sensor_map_init(heat_sensor_map_t *map) {
    if (map == NULL) {
        return;
    }
    memset(map, 0, sizeof(*map));
}

bool heat_sensor_map_set(heat_sensor_map_t *map, const heat_sensor_assignment_t *assignment) {
    if (map == NULL || assignment == NULL) {
        return false;
    }

    if (assignment->port_index >= HEAT_SENSOR_MAP_COUNT) {
        return false;
    }

    heat_sensor_map_entry_t *entry = &map->entries[assignment->port_index];
    entry->index = assignment->port_index;
    entry->node_id = assignment->node_id;
    memcpy(entry->sensor_id, assignment->sensor_id, sizeof(entry->sensor_id));
    entry->sensor_type = assignment->sensor_type;
    entry->source = assignment->source;
    entry->active = assignment->enabled;

    if (map->count <= assignment->port_index) {
        map->count = assignment->port_index + 1;
    }
    return true;
}

bool heat_sensor_map_get(const heat_sensor_map_t *map, uint8_t index, heat_sensor_map_entry_t *entry) {
    if (map == NULL || entry == NULL || index >= HEAT_SENSOR_MAP_COUNT) {
        return false;
    }

    *entry = map->entries[index];
    return entry->active || map->count > index;
}

bool heat_sensor_map_remove(heat_sensor_map_t *map, uint8_t index) {
    if (map == NULL || index >= HEAT_SENSOR_MAP_COUNT) {
        return false;
    }

    memset(&map->entries[index], 0, sizeof(map->entries[index]));
    if (map->count > index) {
        map->count = index;
    }
    return true;
}
