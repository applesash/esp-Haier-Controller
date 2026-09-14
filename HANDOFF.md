# Workspace Handoff

This file is the starting point for a new Copilot session in `esp-Haier-Controller`.

## User Goal

Build an ESP32-S3 controller for a Haier HVAC installation using the Waveshare ESP32-S3-Touch-LCD-4.3B-BOX. The first purpose is observation and display, not HVAC control:

- Read two room temperatures, upstairs and downstairs, from XY modules.
- Investigate what the Haier Roomstat exposes over RS485/Modbus.
- Display the readings and related heating-system values on the wide touch display.

## Overview Design

The overview uses a three-column layout for the wider 800x480 display. The planned tiles are:

1. Outdoor temperature
2. Upstairs room temperature
3. Downstairs room temperature
4. DHW tank temperature
5. Flow temperature
6. Return temperature
7. Flow rate in L/min
8. Thermal output in kW out
9. Thermal input in kW in

The initial visual preview is in `preview/index.html` and `preview/style.css`.

## Current Project State

The folder was empty when generation started. The following scaffold now exists:

- `CMakeLists.txt`: ESP-IDF project root targeting `haier_controller`
- `apps/haier_controller`: initial ESP32-S3 application and partition table
- `common/components/heat_common`: copied reusable shared helpers
- `common/components/heat_modbus`: copied reusable Modbus framing helpers
- `scripts/idf-task.sh`: ESP-IDF wrapper
- `scripts/flash-device.sh`: Haier auto-detect flash helper
- `scripts/git-save-push.sh`: save and push helper for the future online Git repository
- `.vscode/esp-Haier-Controller.code-workspace`: multi-root workspace definition
- `.vscode/settings.json`: ESP-IDF target, setup, compile commands, and approvals
- `.vscode/tasks.json`: build, clean, flash, auto-flash, and save/push tasks
- `docs/hardware.md`: board and electrical bring-up checklist
- `docs/modbus-roomstat.md`: Roomstat investigation rules and capture workflow
- `.github/copilot-instructions.md`: project-specific safety and reuse rules
- `common/components/heat_ch422g`: read-only CH422G input driver and explicit
	output API for the 28141 green terminal
- `common/dashboard/dashboard_model.json`: shared 800x480 dashboard contract
	used by the browser preview and mirrored by the firmware model header
- `docs/hmi-preview-architecture.md`: rules for keeping preview, device web,
	and LVGL surfaces aligned
- `docs/ota.md`: controller-specific HTTPS OTA and rollback plan
- `common/components/heat_web`: SPIFFS-backed web HMI, `haier-hmi` AP,
	status, Wi-Fi scan/connect, and locked commissioning/OTA endpoints
- `common/components/heat_observer`: passive UART RS485 byte observer with no
	transmit path

## Important Decisions

- Start in passive RS485 observation mode.
- Do not transmit Modbus writes until the electrical interface, serial settings, address, register meaning, and recovery behavior are verified.
- Keep Haier-specific register interpretation out of the generic Modbus component.
- Treat the display controller, touch controller, reset/backlight wiring, and BSP
	as unverified until confirmed for the 28141 BOX panel.
- Treat green-terminal outputs as disabled until an explicit hardware test is approved.
- RS485 direction is automatic on the board; TX/RX are GPIO17/GPIO18 and A/B are
	transceiver terminal signals.
- Keep sensor commissioning writes and OTA installation visibly locked until
	their evidence and validation gates are complete.
- The current firmware starts the `haier-hmi` AP, serves the preview from
	SPIFFS, persists Wi-Fi credentials, and exposes read-only status/observation.
- Do not assume the existing `esp-heating-control` 4-inch 480x480 BSP is electrically compatible with this 4.3B-BOX.
- Do not commit build output, `sdkconfig`, serial-port settings, credentials, or raw captures.

## Reuse Context

`heat_common` and `heat_modbus` were copied from `/home/sascha-hlubek/VSC-Workspace/esp-heating-control`. They are useful starting points for CRC, framing, timeout, and protocol diagnostics. Existing boiler, plant, relay, zone-demand, and DHW-control logic was intentionally not copied.

The source project uses ESP-IDF 5.5 and the local installation is expected at `/home/sascha-hlubek/esp/esp-idf`.

## Board Research Note

Public Waveshare sources distinguish the ESP32-S3-Touch-LCD-4.3 and 4.3-B board definitions. The exact 4.3B-BOX display/BSP configuration must be verified before wiring the live LVGL display. Do not silently substitute the existing `waveshare__esp32_s3_touch_lcd_4` component.

## Next Actions

1. Confirm the CH422G address and command behavior on the physical board.
2. Confirm the exact 4.3B-BOX RGB/touch controller and timing.
3. Flash the built firmware and storage image after confirming it is safe to
	reboot the board, then join `haier-hmi` and verify the device web interface.
4. Use the external USB RS485 dongle for passive captures and evidence-backed
	sensor commissioning.
5. Add HTTPS OTA manifest checking, rollback, and shared status reporting.
6. Add LVGL settings/commissioning pages and automated preview/web/LVGL parity checks.
7. Initialize the new folder as its own Git repository and configure its remote.
