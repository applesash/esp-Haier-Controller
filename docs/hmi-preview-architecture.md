# HMI and preview architecture

This project has three synchronized HMI surfaces:

1. The browser preview in `preview/`.
2. The device web interface, which will serve the same assets from flash.
3. The 800x480 LVGL panel on the Waveshare 28141 BOX board.

The preview and device web interface use the same HTML, CSS, JavaScript, and
dashboard model. The LVGL panel is a native C implementation that mirrors the
same model, labels, settings categories, commissioning stages, and interaction
states. The three surfaces are one product surface, not three independent UIs.

The same low-credit workflow applies to firmware work: use
`scripts/haier-controller.sh` for environment checks, builds, flashing,
monitoring, and preview serving. Do not repeat raw `idf.py` setup commands in
feature instructions when a wrapper command already exists.

## Shared model

`common/dashboard/dashboard_model.json` is the preview seed and API-shaped
contract. `common/dashboard/dashboard_model.h` defines the embedded constants
and tile IDs. Any new status, setting, commissioning stage, Wi-Fi field, or OTA
field belongs in this contract first.

The preview may simulate values when opened without a device, but it must label
simulated actions clearly. It must not imply that a Modbus write, Wi-Fi
connection, or OTA install happened on real hardware.

## Alignment rules

- Put structure and rendering in the shared preview files, never in a live
  adapter-specific branch.
- The preview is the interaction specification. A new page, settings category,
  tile state, control, or error state is added there first.
- Keep values in the dashboard model; do not duplicate device defaults as
  unrelated literals in HTML, LVGL code, and web handlers.
- The device web UI serves those same preview assets from SPIFFS. It may replace
  the local seed adapter with API data, but it must not fork the markup or
  rendering logic.
- The LVGL implementation mirrors the preview structure and uses the same tile
  IDs, settings IDs, labels, ranges, status states, and action names. Native C
  widgets are an implementation of the shared contract, not a second design.
- Keep commissioning as an explicit state machine: `idle`, `detect`, `verify`,
  and later evidence-backed `change` or `calibrate` stages.
- The first RS485 workflow is observation-only. Preview buttons can simulate
  the workflow, but embedded actions must not transmit until the external
  dongle captures establish the protocol and safety conditions.
- Wi-Fi provisioning must expose access-point fallback, scan, connect, and
  connection status in the same model.
- OTA must expose channel, current version, manifest status, and update status.
  Installation remains unavailable until signed HTTPS delivery and rollback
  behavior are implemented.
- After changing preview structure, update the LVGL page and web API contract
  before calling the feature complete.
- Do not call an HMI feature complete until it works in the preview, is served
  by the device web interface, and is represented on the LVGL/touch panel.

## Current preview capabilities

The Settings view currently models:

- RS485 read-only observation parameters
- Wi-Fi access-point/provisioning state
- OTA stable/beta channel state and manifest checks
- RS485 sensor role/address commissioning with detect and verify actions
- Display brightness, rotation, and touch state
- Disabled green-terminal outputs with an explicit hardware gate

The Wi-Fi, OTA, and commissioning buttons are intentionally preview-only until
corresponding device APIs are implemented.

## Display and touch implementation

The 28141 panel uses the official LVGL 8 library with the official Waveshare
BOX hardware transport imported from the vendor demo. `board_28141.h` remains
our board contract and the HMI renderer remains project-owned. GT911 touch is
enabled through the CH422G reset sequence and interrupt-free polling path.
Physical touch behavior and display stability must still be validated on the
panel before this is considered complete.

The display bring-up sequence is:

1. Verify the 28141 RGB data, clock, sync, DE, reset, and backlight mapping.
2. Verify touch SDA/SCL, interrupt, reset, and controller address.
3. Show a panel color/test pattern before starting the full LVGL dashboard.
4. Start LVGL at 800x480 and register touch input.
5. Mirror the preview overview and settings sub-screens.
6. Validate touch navigation against the same actions exposed by the web UI.

## Implementation order

1. Add a device web server that serves `preview/` from a flash storage
   partition and exposes JSON endpoints matching the dashboard model.
2. Add Wi-Fi station/AP management and persist approved settings.
3. Add external-dongle capture import and evidence-backed sensor commissioning.
4. Add HTTPS OTA manifest checking, image validation, rollback, and status
   reporting.
5. Implement the verified 28141 RGB/LVGL/touch bring-up and test pattern.
6. Mirror the same settings and commissioning pages in LVGL using the shared
  model IDs and labels.
7. Add parity checks so preview, web, and LVGL cannot silently lose a setting.
