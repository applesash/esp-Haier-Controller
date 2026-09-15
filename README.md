# ESP Haier Controller
The same dashboard model is intended for the device web interface and the LVGL HMI.

ESP32-S3 project for observing a Haier HVAC installation through RS485/Modbus and displaying two room temperatures on the Waveshare ESP32-S3-Touch-LCD-4.3B-BOX (SKU 28141).

## Current scope

- Upstairs and downstairs room temperature inputs
- Outdoor, DHW tank, flow, and return temperature tiles
- Planned flow rate (L/min), thermal output (kW out), and thermal input (kW in) tiles
- Three-column overview for the 800x480 display
- Passive RS485 observation before any Roomstat writes

The first firmware scaffold is intentionally read-only. The Haier Roomstat register map, serial settings, addressing, and electrical bus behavior must be verified from captures before control writes are enabled.

## Layout

- `docs/hmi-preview-architecture.md`: preview, device web, and LVGL alignment rules
- `docs/web-api.md`: shared device-web API contract for Wi-Fi, commissioning, and OTA
- `docs/ota.md`: controller-specific HTTPS OTA and rollback plan
- `scripts/`: build, flash, and host-test helpers

## Build

```sh
bash scripts/idf-task.sh apps/haier_controller build
```

## Repeatable workflow

Use the project wrapper for routine operations:

```sh
scripts/haier-controller.sh env
scripts/haier-controller.sh build
scripts/haier-controller.sh check
scripts/haier-controller.sh monitor /dev/ttyACM0
scripts/haier-controller.sh build-flash /dev/ttyACM0
scripts/haier-controller.sh preview
```

This wrapper is the low-credit workflow entry point, following the adjacent
project's `setup-env.sh` and `idf-task.sh` pattern. `build` creates both the
firmware image and `storage.bin`. `flash` and `build-flash` are the only
commands that write to the board. `monitor` reads the USB serial log, including
passive RS485 bytes. The VS Code tasks expose the same operations.

The target is `esp32s3`. Display and touch bring-up remains behind the board adapter until the 4.3B-BOX panel configuration is verified.

The preview Settings page models Wi-Fi provisioning, OTA channel/check state,
and staged RS485 sensor commissioning. These actions are simulated until the
device web API and external-dongle protocol evidence are in place. The same
dashboard model is intended for the device web interface and LVGL HMI.
