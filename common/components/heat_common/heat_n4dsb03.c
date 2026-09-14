#include "heat_n4dsb03.h"

#include <stddef.h>

bool heat_n4dsb03_decode_temperatures(const uint16_t registers[HEAT_N4DSB03_CHANNEL_COUNT],
                                      heat_n4dsb03_reading_t *reading) {
    if (registers == NULL || reading == NULL) {
        return false;
    }
    for (uint8_t channel = 0; channel < HEAT_N4DSB03_CHANNEL_COUNT; ++channel) {
        reading->channels[channel].temperature_deci_c = (int16_t)registers[channel];
        const int16_t temperature_deci_c = reading->channels[channel].temperature_deci_c;
        reading->channels[channel].valid = registers[channel] != 0x7FFFu &&
            registers[channel] != 0x8000u &&
            temperature_deci_c >= HEAT_N4DSB03_MIN_TEMPERATURE_DECI_C &&
            temperature_deci_c <= HEAT_N4DSB03_MAX_TEMPERATURE_DECI_C;
    }
    return true;
}

bool heat_n4dsb03_address_valid(uint8_t address) {
    return address >= 1u && address <= 247u;
}