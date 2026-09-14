# OTA update plan

The controller will use HTTPS OTA with a channel-based manifest, following the
sibling `esp-heating-control` project.

## Channels

- `stable`: production releases
- `beta`: investigation and preview releases

The dashboard model exposes the selected channel, current firmware version,
manifest status, and whether an update is available. The preview simulates
checks and never downloads or installs firmware.

## Required device behavior

1. Fetch a channel manifest over HTTPS with certificate validation.
2. Select the firmware image for the ESP32-S3 Haier controller target.
3. Verify image metadata and checksum before activation.
4. Write through the ESP-IDF OTA partition API.
5. Reboot and confirm the new image.
6. Mark the image valid only after successful startup; otherwise use rollback.
7. Report progress, failure, and current version through the shared dashboard
   model and device web API.

OTA installation must remain disabled until Wi-Fi management, HTTPS certificate
handling, partition layout, and rollback behavior are implemented and tested.
The project must not silently reuse the sibling project's firmware target or
manifest URLs.
