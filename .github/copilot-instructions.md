# Project Guidelines

- Read `HANDOFF.md` first when starting a new session; it records the current
	project state, design decisions, and next actions.
- This is an ESP-IDF v5.5 ESP32-S3 Haier Roomstat investigation project.
- Begin with passive RS485 observation and read-only decoding. Do not add writes without documented evidence and an explicit safety gate.
- Keep the two room sources independently mapped as upstairs and downstairs.
- Preserve the wide 800x480 three-column overview: outdoor, upstairs, downstairs, DHW tank, flow, return, flow rate, thermal output, and thermal input.
- Reuse `common/components/heat_modbus` for framing, CRC, timeout, and diagnostics. Keep Haier register interpretation in Haier-specific code and docs.
- Treat the official 28141 BOX RGB mapping and transport as the verified HMI
	baseline. Keep GT911 touch disabled until its polling path is independently
	fixed without destabilizing the screen.
- Do not commit `build/`, `sdkconfig`, captures, credentials, or local serial-port settings.
- Use `scripts/haier-controller.sh env` for environment checks, `build` for
	firmware and SPIFFS, `check` for artifacts, `monitor` for serial logs, and
	`build-flash` only when flashing is explicitly requested.
- Keep preview, device web, and LVGL/touch HMI changes synchronized through
	`common/dashboard/dashboard_model.json` and
	`docs/hmi-preview-architecture.md`.
