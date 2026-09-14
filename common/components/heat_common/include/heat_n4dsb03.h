#pragma once

#include <stdbool.h>
#include <stdint.h>

/* Provisional values from the N4DSB03 seller documentation. Verify these
 * against the physical module before enabling live polling. */
#define HEAT_N4DSB03_DEFAULT_ADDRESS 0x01u
#define HEAT_N4DSB03_ADDRESS_REGISTER 0x000Fu
#define HEAT_N4DSB03_TEMPERATURE_REGISTER 0x0000u
#define HEAT_N4DSB03_CHANNEL_COUNT 2u
#define HEAT_N4DSB03_BAUD_RATE 9600u
#define HEAT_N4DSB03_MIN_TEMPERATURE_DECI_C (-550)
#define HEAT_N4DSB03_MAX_TEMPERATURE_DECI_C 1250

typedef struct {
    int16_t temperature_deci_c;
    bool valid;
} heat_n4dsb03_channel_t;

typedef struct {
    heat_n4dsb03_channel_t channels[HEAT_N4DSB03_CHANNEL_COUNT];
} heat_n4dsb03_reading_t;

/* Decode the two consecutive signed tenths-degree registers returned by an
 * N4DSB03. Out-of-range values and common disconnected-probe sentinels are
 * marked invalid while the other channel remains independently usable. */
bool heat_n4dsb03_decode_temperatures(const uint16_t registers[HEAT_N4DSB03_CHANNEL_COUNT],
                                      heat_n4dsb03_reading_t *reading);

/* Validate a normal Modbus slave address for commissioning. Address zero is
 * broadcast and must never be stored as a sensor address. */
bool heat_n4dsb03_address_valid(uint8_t address);