# KodeDotBSP

Board Support Package for ESP32-S3 devices: display bring-up, LVGL integration, and capacitive touch.

## Supported Devices

### Kode Dot (Primary Target)
- **Display**: 410x502 QSPI LCD with ST7701 driver
- **Touch**: CST816S capacitive touch controller
- **SD Card**: SDMMC mode (4-bit)
- **Microphone**: I2S MEMS (SPH0645LM4H)
- **Features**: RGB LED strip, PMIC (BQ25896)

### M5Stack CoreS3-Lite (Added in Port)
- **Display**: 320x240 IPS LCD via M5Unified
- **Touch**: FT6336U capacitive via M5Unified
- **SD Card**: SPI mode (managed by M5Unified)
- **Microphone**: ES7210 digital I2S (4-channel ADC)
- **Features**: Battery management (AXP2101), IMU (BMI270)
- **M5Unified Version**: Requires 0.1.16-0.1.x (compile-time check enforced)
- **GPIO Availability**: 12 user pins (1,2,5,6,7,8,9,10,11,12,13,18)
  - **GPIO 14 Reserved**: Used by ES7210 microphone I2S DIN signal, excluded from user control to prevent audio conflicts

### M5Stack Cardputer
- **Display**: 240x135 IPS LCD via M5Unified
- **Touch**: Internal button matrix
- **SD Card**: SPI mode with explicit pin configuration
- **Keyboard**: Physical QWERTY keyboard

## Usage

Include the public headers with the `kodedot/` prefix:

```cpp
#include <kodedot/display_manager.h>
#include <kodedot/pin_config.h>
```

Initialize once in `setup()` and update in `loop()`:

```cpp
DisplayManager display;

void setup() {
  Serial.begin(115200);
  if (!display.init()) {
    while (1) delay(1000);
  }
}

void loop() {
  display.update();
  delay(5);
}
```

`pin_config.h` exposes board-specific pins and constants, automatically configured via conditional compilation:

```cpp
#ifdef CORES3_LITE_TARGET
  // CoreS3-Lite pin definitions
#elif defined(CARDPUTER_TARGET)
  // Cardputer pin definitions
#else
  // Kode Dot pin definitions (default)
#endif
```

## Pin Mappings

### CoreS3-Lite Pin Configuration
The CoreS3-Lite uses the following pin assignments:

**Display & Touch**: Managed by M5Unified library
- **LCD**: 320x240 IPS via internal bus
- **Touch Controller**: FT6336U via M5.Touch API

**Audio (I2S)**:
- `MIC_I2S_SCK`: GPIO 34 (Bit Clock)
- `MIC_I2S_WS`: GPIO 33 (Word Select/LRCK)
- `MIC_I2S_DIN`: GPIO 14 (Data In) - **RESERVED, DO NOT USE FOR GPIO**

**SD Card**: SPI3_HOST managed by M5Unified
- Initialization handled automatically via `M5.begin()`

**User-Available GPIOs**: 1, 2, 5, 6, 7, 8, 9, 10, 11, 12, 13, 18
- **Total**: 12 pins available for GPIO/Servo control
- **Exclusion Rationale**: GPIO 14 excluded due to ES7210 microphone I2S DIN conflict

**Battery**: AXP2101 PMIC via M5.Power API
- `M5.Power.getBatteryLevel()` - Read battery percentage
- `M5.Power.isCharging()` - Check charging status

## Notes
- Prefers PSRAM for LVGL draw buffers; falls back to internal SRAM.
- Registers LVGL display and input drivers.
- Provides simple brightness and touch helpers.
- M5Unified-based targets (CoreS3-Lite, Cardputer) use M5.begin() for hardware initialization.
