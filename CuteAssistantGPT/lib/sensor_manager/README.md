# Sensor Manager Library

Unified sensor management library for CuteAssistantGPT with automatic I2C scanning and placeholder-based sensor reading.

## Supported Sensors

### BME688 Environmental Sensor (I2C: 0x76/0x77)
- **Temperature**: Celsius, ±1°C accuracy
- **Humidity**: Relative humidity %, ±3% accuracy
- **Pressure**: Atmospheric pressure in hPa, ±1 hPa accuracy
- **Gas Resistance**: Air quality indicator in ohms

**Library**: `Adafruit_BME680`

### ADXL345 3-Axis Accelerometer (I2C: 0x53)
- **Acceleration**: 3-axis measurement in g (gravity units)
- **Range**: ±16g
- **Resolution**: 0.01g

**Library**: `Adafruit_ADXL345_U`

### RFID 2 UNIT Reader (I2C: 0x28)
- **Function**: Read RFID/NFC card UIDs
- **Format**: Hex string (e.g., "04:A3:2C:BA:FF:01")
- **Compatible Cards**: MIFARE Classic, MIFARE Ultralight

**Hardware**: M5Stack RFID 2 UNIT (MFRC522-based)

### Hall Effect Sensor (GPIO)
- **Function**: Magnetic field detection
- **Output**: Digital (active-low)
- **Pin**: Configurable (default: GPIO5)

## Usage

### Initialization
```cpp
#include <sensor_manager/SensorManager.h>

SensorManager sensorManager;

void setup() {
    // Initialize with I2C pins (default: SDA=11, SCL=12)
    if (sensorManager.init()) {
        Serial.printf("Sensors: %s\n", sensorManager.getAvailableSensors().c_str());
    }
}
```

### Direct Sensor Access
```cpp
// Read BME688 temperature
if (BME688Sensor* bme = sensorManager.getBME688()) {
    float temp;
    if (bme->readTemperature(temp)) {
        Serial.printf("Temperature: %.1f°C\n", temp);
    }
}

// Read accelerometer
if (ADXL345Sensor* accel = sensorManager.getADXL345()) {
    float x, y, z;
    if (accel->readAcceleration(x, y, z)) {
        Serial.printf("Accel: X=%.2f Y=%.2f Z=%.2fg\n", x, y, z);
    }
}
```

### Placeholder-Based Reading (for AI Integration)
```cpp
// Read sensor by string query
String temp = sensorManager.readSensorByString("bme688", "temperature");
// Returns: "23.5" (or "[BME688 not available]" if sensor missing)

String accelX = sensorManager.readSensorByString("adxl345", "x");
// Returns: "0.82" (acceleration in g)

String magnet = sensorManager.readSensorByString("hall", "detect");
// Returns: "detected" or "not detected"

String card = sensorManager.readSensorByString("rfid", "uid");
// Returns: "04:A3:2C:BA" or "no card"
```

## Placeholder Format

For AI-powered conversational sensor readings, use placeholder syntax:

```
[SENSOR:device:parameter]
```

**Examples**:
- `[SENSOR:bme688:temperature]` → "23.5"
- `[SENSOR:bme688:humidity]` → "45.2"
- `[SENSOR:bme688:pressure]` → "1013.2"
- `[SENSOR:bme688:gas]` → "245830"
- `[SENSOR:bme688:all]` → "23.5°C, 45.2%, 1013.2hPa"
- `[SENSOR:adxl345:x]` → "0.82"
- `[SENSOR:adxl345:y]` → "-0.15"
- `[SENSOR:adxl345:z]` → "9.81"
- `[SENSOR:adxl345:all]` → "X:0.82g, Y:-0.15g, Z:9.81g"
- `[SENSOR:hall:detect]` → "detected" / "not detected"
- `[SENSOR:rfid:uid]` → "04:A3:2C:BA:FF:01" / "no card"

## AI Integration Example

### System Prompt
```
When user asks about sensors, use placeholder syntax: [SENSOR:device:parameter]

User: "What's the temperature?"
You: "The current temperature is [SENSOR:bme688:temperature] degrees Celsius."

User: "Am I tilted?"
You: "You're tilted at [SENSOR:adxl345:x] g on the X axis."
```

### Processing in main.cpp
```cpp
String gptResponse = "The temperature is [SENSOR:bme688:temperature]°C";
String finalResponse = replaceSensorPlaceholders(gptResponse);
// Result: "The temperature is 23.5°C"
```

## Error Handling

**Sensor Not Available**:
```cpp
String result = sensorManager.readSensorByString("bme688", "temperature");
// Returns: "[BME688 not available]" if sensor not detected
```

**Read Error**:
```cpp
// Returns: "[BME688 read error]" if sensor present but read failed
```

**Unknown Sensor**:
```cpp
String result = sensorManager.readSensorByString("invalid", "param");
// Returns: "[Unknown sensor: invalid]"
```

## I2C Bus Configuration

CoreS3-Lite internal I2C:
- **SDA**: GPIO 11
- **SCL**: GPIO 12
- **Clock**: 100kHz (for maximum compatibility)

**Note**: All I2C sensors share the same bus. Ensure no address conflicts.

## Adding New Sensors

1. Create sensor class (e.g., `NewSensor.h/cpp`)
2. Add to `SensorManager.h`:
   ```cpp
   NewSensor* getNewSensor();
   private:
   NewSensor* newSensor_;
   bool newSensorAvailable_;
   ```
3. Add detection in `SensorManager::detectSensors()`
4. Add handler in `readSensorByString()`:
   ```cpp
   else if (deviceLower == "newsensor") {
       // Handle reading
   }
   ```
5. Update system prompt with new sensor examples

## Dependencies
```ini
lib_deps =
    adafruit/Adafruit BME680 Library @ ^2.0.4
    adafruit/Adafruit ADXL345 @ ^1.3.4
    adafruit/Adafruit Unified Sensor @ ^1.1.4
```

## Example Conversations

**Temperature Query**:
```
User: "What's the temperature?"
AI: "The current temperature is 24.2 degrees Celsius."
[Behind the scenes: [SENSOR:bme688:temperature] → "24.2"]
```

**Tilt Detection**:
```
User: "Am I tilting the device?"
AI: "Yes, you're tilted at 0.8g on the X axis."
[Behind the scenes: [SENSOR:adxl345:x] → "0.8"]
```

**RFID Card Reading**:
```
User: "Is there a card nearby?"
AI: "Yes, I detect an RFID card with UID: 04:A3:2C:BA:FF:01"
[Behind the scenes: [SENSOR:rfid:uid] → "04:A3:2C:BA:FF:01"]
```

**Magnetic Field**:
```
User: "Do you sense a magnet?"
AI: "Yes! Magnetic field is detected."
[Behind the scenes: [SENSOR:hall:detect] → "detected"]
```

## Limitations
- I2C bus shared between all sensors (potential conflicts)
- Hall Effect sensor requires external wiring to Grove port
- BME688 gas sensor requires 1+ second measurement time
- RFID reader range: ~5cm maximum
- ADXL345 sampling rate: ~100Hz max in this implementation

## Troubleshooting

**No sensors detected**:
- Check I2C wiring (SDA/SCL pins)
- Verify sensor I2C address with `i2cdetect` tool
- Ensure sensor power supply (3.3V for most modules)

**Incorrect readings**:
- BME688: Wait 1 second minimum between reads
- ADXL345: Calibrate on flat surface (Z should be ~9.8g)
- Hall Effect: Check active-low vs active-high configuration

**I2C communication errors**:
- Reduce I2C clock speed if long wires
- Add pull-up resistors (4.7kΩ) to SDA/SCL
- Check for address conflicts between sensors
