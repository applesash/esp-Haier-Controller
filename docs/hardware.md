# Hardware Notes

## Target

- MCU: ESP32-S3
- Display: Waveshare ESP32-S3-Touch-LCD-4.3B-BOX (SKU 28141)
- Display class: wide 800x480 touch panel, confirmed as the box variant for this project
- Bus under investigation: Haier Roomstat RS485/Modbus

Official board documentation: [Waveshare ESP32-S3-Touch-LCD-4.3B](https://docs.waveshare.com/ESP32-S3-Touch-LCD-4.3B)
ESP-IDF reference: [Waveshare ESP32-S3-Touch-LCD-4.3B ESP-IDF](https://docs.waveshare.com/ESP32-S3-Touch-LCD-4.3B/ESP-IDF)

## Bring-up checklist

- Board target is the Waveshare 28141 BOX variant; display controller and RGB
	timing still require panel-level confirmation.
- The shared I2C bus is used by the touch device, external I2C, and CH422G
	expander on the board.
- CH422G terminal channels are DI0/DI1 on EXIO0/EXIO5 and DO0/DO1 on OD0/OD1.
- RS485 uses automatic transceiver direction; ESP32 UART pins are TX=44 and
	RX=43. GPIO17/18 are RGB LCD data lines on the 28141 and must not be used
	for RS485. A/B are transceiver terminal signals, not ESP32 GPIOs.
- Confirm touch controller and reset/backlight wiring.
- Confirm RS485 transceiver isolation and A/B polarity.
- Confirm whether the Haier bus permits an additional listener.
- Record UART pins and electrical levels before enabling the bus task.
- Firmware web access is configured through the open `haier-hmi` access point;
	connect to `http://192.168.4.1/` after flashing.

## Verification status

Current status: the 28141 BOX identity, 800x480 RGB display, automatic RS485
direction, CH422G digital I/O channel names, and official vendor RGB transport
are verified. GT911 identification succeeds, but touch data reads fail on the
current ESP-IDF path, so the official display-only baseline remains enabled.

Required evidence before display or RS485 wiring:

- Official pinout/schematic for the board adapter and the LCD panel connector.
- Touch controller polling, reset/address sequence, and interrupt behavior.
- Verified RS485 transceiver A/B pin mapping and whether the bus is isolated.
- CH422G I2C address and command behavior confirmed against the physical board.

Until the remaining touch evidence exists, keep the stable display-only baseline.
The firmware CH422G probe is read-only, the RS485 observer never transmits except
for the explicitly ported commissioning workflow, and OTA installation remains
locked.
