# testScreen

`apps/testScreen` is a deliberately isolated display bring-up application for the Waveshare ESP32-S3-Touch-LCD-4.3B-BOX, SKU 28141.

Its only UI is:

- one `Hello, world!` line
- one button below it
- pressing the button changes the line to `Button pressed`

The app uses ESP-IDF v5.5.x APIs and LVGL 8.4.0 only. It contains no Waveshare sample source, BSP, or legacy driver. Board I/O is project-owned and uses the documented 28141 RGB pins plus the modern ESP-IDF I2C master API for the CH422G backlight.

The pin assignments are based on the official board documentation and schematic:

- RGB: GPIO3/46/5/7 for VSYNC/HSYNC/DE/PCLK
- RGB data: GPIO14, 38, 18, 17, 10, 39, 0, 45, 48, 47, 21, 1, 2, 42, 41, 40
- I2C: SDA GPIO8, SCL GPIO9
- CH422G control: `0x24`
- CH422G output: `0x38`

Use the wrapper commands from the repository root:

```text
scripts/haier-controller.sh test-build
scripts/haier-controller.sh test-build-flash /dev/ttyACM0
scripts/haier-controller.sh test-monitor /dev/ttyACM0
```

The production `apps/haier_controller` app and the isolated test app are separate build targets. Do not copy production HMI, RS485, web, touch, or Waveshare sample code into `testScreen`.
