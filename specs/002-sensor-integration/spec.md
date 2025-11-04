# Sensor Integration with OpenAI Function Calling

## Overview
Add support for multiple I2C/GPIO sensors with OpenAI Function Calling API to enable conversational sensor readings.

## User Story
**As a user**, I want to ask the voice assistant "What's the temperature?" or "Is there movement?" and get real-time sensor readings spoken back to me.

## Supported Sensors

### 1. BME688 Environmental Sensor (I2C)
- **Functions**: Temperature, Humidity, Pressure, Gas Resistance (air quality)
- **Address**: 0x76 or 0x77
- **Library**: `Adafruit_BME680` or `bsec` (Bosch)
- **Pins**: SDA=GPIO11, SCL=GPIO12 (CoreS3-Lite internal I2C)

### 2. ADXL345 Accelerometer (I2C)
- **Functions**: 3-axis acceleration, tilt detection, tap detection
- **Address**: 0x53 or 0x1D
- **Library**: `Adafruit_ADXL345`
- **Pins**: SDA=GPIO11, SCL=GPIO12

### 3. RFID 2 UNIT (MFRC522) (I2C)
- **Functions**: Read RFID/NFC card UID, read/write blocks
- **Address**: 0x28
- **Library**: `M5Unit-RFID2` or custom I2C driver
- **Pins**: SDA=GPIO11, SCL=GPIO12

### 4. Hall Effect Unit (GPIO)
- **Functions**: Magnetic field detection (digital output)
- **Library**: None (simple digitalRead)
- **Pins**: Any available GPIO (e.g., GPIO5)

### 5. Atomic QRCode2 Base (UART Camera)
- **Functions**: QR code scanning
- **Interface**: UART (TX/RX)
- **Library**: M5-specific QR decoder
- **Pins**: UART2 (GPIO17/TX, GPIO18/RX)

### 6. Mini PDM Microphone Unit
- **Note**: Already integrated for voice input
- **Future**: Could add sound level metering function

## Architecture

### Function Calling Flow
```
User: "What's the temperature?"
  ↓
OpenAI GPT-4 + Functions schema
  ↓
Function Call: {"name": "read_bme688", "arguments": {"sensor": "temperature"}}
  ↓
Device executes: SensorManager.readBME688()
  ↓
Returns: {"temperature": 23.5}
  ↓
Second API call with function result
  ↓
GPT: "The current temperature is 23.5 degrees Celsius"
  ↓
TTS output to user
```

### OpenAI Function Schema
```json
{
  "functions": [
    {
      "name": "read_bme688",
      "description": "Read environmental sensor data (temperature, humidity, pressure, air quality)",
      "parameters": {
        "type": "object",
        "properties": {
          "sensor": {
            "type": "string",
            "enum": ["temperature", "humidity", "pressure", "gas", "all"]
          }
        },
        "required": ["sensor"]
      }
    },
    {
      "name": "read_accelerometer",
      "description": "Read 3-axis acceleration data from ADXL345",
      "parameters": {
        "type": "object",
        "properties": {
          "axis": {
            "type": "string",
            "enum": ["x", "y", "z", "all"]
          }
        }
      }
    },
    {
      "name": "detect_magnetic_field",
      "description": "Check if magnetic field is detected by Hall Effect sensor",
      "parameters": {"type": "object", "properties": {}}
    },
    {
      "name": "read_rfid_card",
      "description": "Read RFID/NFC card UID if present",
      "parameters": {"type": "object", "properties": {}}
    },
    {
      "name": "scan_qr_code",
      "description": "Scan and decode QR code from camera",
      "parameters": {"type": "object", "properties": {}}
    }
  ]
}
```

## Implementation Plan

### Phase 1: Core Infrastructure
1. Create `sensor_manager` library
2. Add Function Calling support to `BasicGPTClient`
3. Implement I2C bus initialization

### Phase 2: Sensor Drivers
1. BME688 integration
2. ADXL345 integration
3. Hall Effect (simple GPIO)
4. RFID2 integration
5. QRCode2 integration (optional)

### Phase 3: Function Calling Protocol
1. Parse function call response from OpenAI
2. Execute sensor read function
3. Format result as JSON
4. Send follow-up request with function result
5. Handle multi-turn function calling

## File Structure
```
CuteAssistantGPT/
├── lib/
│   ├── sensor_manager/
│   │   ├── include/sensor_manager/
│   │   │   ├── SensorManager.h
│   │   │   ├── BME688Sensor.h
│   │   │   ├── ADXL345Sensor.h
│   │   │   ├── RFIDSensor.h
│   │   │   └── HallSensor.h
│   │   └── src/
│   │       ├── SensorManager.cpp
│   │       ├── BME688Sensor.cpp
│   │       ├── ADXL345Sensor.cpp
│   │       ├── RFIDSensor.cpp
│   │       └── HallSensor.cpp
│   └── basicgpt_client/
│       └── (add Function Calling support)
└── platformio.ini (add sensor libraries)
```

## Dependencies
```ini
lib_deps =
    adafruit/Adafruit BME680 Library @ ^2.0.4
    adafruit/Adafruit ADXL345 @ ^1.3.4
    m5stack/M5Unit-RFID2 @ ^0.0.3  # If available
```

## Example Conversations

**Temperature Check**:
```
User: "What's the temperature?"
Assistant: "The current temperature is 24.2 degrees Celsius."
[Function: read_bme688(sensor="temperature") → 24.2]
```

**Tilt Detection**:
```
User: "Am I tilting the device?"
Assistant: "Yes, you're tilting it forward. The X-axis shows +0.8g acceleration."
[Function: read_accelerometer(axis="all") → {x: 0.8, y: 0.1, z: 9.8}]
```

**RFID Card**:
```
User: "Is there a card nearby?"
Assistant: "Yes, I detect an RFID card with UID: 04:A3:2C:BA:FF:01"
[Function: read_rfid_card() → "04:A3:2C:BA:FF:01"]
```

**Magnetic Detection**:
```
User: "Do you sense a magnet?"
Assistant: "Yes! I'm detecting a strong magnetic field right now."
[Function: detect_magnetic_field() → true]
```

## Success Criteria
- [x] User can ask about temperature and get spoken response
- [x] User can query any sensor via natural language
- [x] Function calling handles multi-turn conversations
- [x] All sensors work independently without conflicts
- [x] I2C bus properly shared between sensors
- [x] Graceful handling of disconnected sensors

## Notes
- CoreS3-Lite has only one I2C bus (internal: SDA=11, SCL=12)
- Need I2C address conflict resolution if multiple sensors use same address
- Hall Effect sensor requires external connection to Grove port
- QRCode2 requires separate UART port (may conflict with debug serial)
