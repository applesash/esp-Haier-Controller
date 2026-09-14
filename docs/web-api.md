# Device web API contract

The device web interface will serve the files in `preview/` from the `storage`
SPIFFS partition. The browser preview and the device web UI must use the same
rendering code. Only the live adapter changes from local simulation to these
endpoints.

## Endpoints

- `GET /api/status`
  - Returns the dashboard model from `common/dashboard/dashboard_model.json`
    shape, including status, readings, Wi-Fi, OTA, and commissioning state.
- `GET /api/wifi/scan`
  - Returns visible SSIDs and signal information.
- `POST /api/wifi/connect`
  - Accepts a device name, SSID, and credentials over the authenticated local
    settings path. The device keeps the `haier-hmi` access point available until
    station connection succeeds.
- `POST /api/modbus/commission`
  - Accepts `detect` or `verify`, sensor role, current address, and target
    address. No address-changing step is enabled until dongle captures verify
    the sensor protocol and recovery behavior.
- `GET /api/ota/check?channel=stable|beta`
  - Checks the controller-specific signed HTTPS manifest and returns version,
    checksum, and availability.
- `POST /api/ota/apply`
  - Starts a validated OTA installation. This endpoint remains disabled until
    HTTPS certificate validation, image checks, rollback, and boot validation
    are implemented.

## Safety and parity

The preview uses the same request names and response shape but simulates actions
when no device is connected. The device adapter must not add markup or duplicate
rendering logic. Any new endpoint field is added to the shared dashboard model
first, then mirrored in the LVGL model and its settings/diagnostics page.

The first firmware slice already serves the static preview from SPIFFS, starts
the `haier-hmi` access point, supports Wi-Fi scan/connect persistence, and
returns status. Sensor commissioning and OTA application intentionally return a
locked response until their evidence and validation gates are complete.
