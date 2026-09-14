# HMI and preview architecture

This project has three intended HMI surfaces:

1. The browser preview in `preview/`.
2. The device web interface, which will serve the same assets from flash.
3. The 800x480 LVGL panel on the Waveshare 28141 BOX board.

The preview and device web interface must use the same HTML, CSS, JavaScript,
and dashboard model. The LVGL panel is a native C implementation and mirrors
the same model and workflows.

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
- Keep values in the dashboard model; do not duplicate device defaults as
  unrelated literals in HTML, LVGL code, and web handlers.
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

## Implementation order

1. Add a device web server that serves `preview/` from a flash storage
   partition and exposes JSON endpoints matching the dashboard model.
2. Add Wi-Fi station/AP management and persist approved settings.
3. Add external-dongle capture import and evidence-backed sensor commissioning.
4. Add HTTPS OTA manifest checking, image validation, rollback, and status
   reporting.
5. Mirror the same settings and commissioning pages in LVGL using the shared
   model IDs and labels.
6. Add parity checks so preview, web, and LVGL cannot silently lose a setting.
