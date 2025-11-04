#include "sensor_manager/SensorManager.h"
#include "sensor_manager/BME688Sensor.h"
#include "sensor_manager/ADXL345Sensor.h"
#include "sensor_manager/RFIDSensor.h"
#include "sensor_manager/HallSensor.h"

SensorManager::SensorManager()
    : bme688_(nullptr)
    , adxl345_(nullptr)
    , rfid_(nullptr)
    , hall_(nullptr)
    , sdaPin_(11)
    , sclPin_(12)
    , bme688Available_(false)
    , adxl345Available_(false)
    , rfidAvailable_(false)
    , hallAvailable_(false) {
}

SensorManager::~SensorManager() {
    if (bme688_) delete bme688_;
    if (adxl345_) delete adxl345_;
    if (rfid_) delete rfid_;
    if (hall_) delete hall_;
}

bool SensorManager::init(int sda, int scl) {
    sdaPin_ = sda;
    sclPin_ = scl;

    Serial.println("[SensorManager] Initializing I2C bus...");
    Wire.begin(sdaPin_, sclPin_);
    Wire.setClock(100000); // 100kHz for compatibility
    delay(100);

    Serial.println("[SensorManager] Scanning for sensors...");
    bool anyDetected = detectSensors();

    if (anyDetected) {
        Serial.printf("[SensorManager] Detected sensors: %s\n", getAvailableSensors().c_str());
    } else {
        Serial.println("[SensorManager] WARNING: No sensors detected");
    }

    return anyDetected;
}

bool SensorManager::detectSensors() {
    bool anyFound = false;

    // Try BME688 at both possible addresses
    if (scanI2CAddress(0x77) || scanI2CAddress(0x76)) {
        bme688_ = new BME688Sensor();
        uint8_t addr = scanI2CAddress(0x77) ? 0x77 : 0x76;
        if (bme688_->begin(&Wire, addr)) {
            bme688Available_ = true;
            anyFound = true;
            Serial.printf("[SensorManager] BME688 found at 0x%02X\n", addr);
        } else {
            delete bme688_;
            bme688_ = nullptr;
        }
    }

    // Try ADXL345 at 0x53 (ALT ADDRESS LOW)
    if (scanI2CAddress(0x53)) {
        adxl345_ = new ADXL345Sensor();
        if (adxl345_->begin(&Wire)) {
            adxl345Available_ = true;
            anyFound = true;
            Serial.println("[SensorManager] ADXL345 found at 0x53");
        } else {
            delete adxl345_;
            adxl345_ = nullptr;
        }
    }

    // Try RFID 2 UNIT at 0x28
    if (scanI2CAddress(0x28)) {
        rfid_ = new RFIDSensor();
        if (rfid_->begin(&Wire, 0x28)) {
            rfidAvailable_ = true;
            anyFound = true;
            Serial.println("[SensorManager] RFID 2 UNIT found at 0x28");
        } else {
            delete rfid_;
            rfid_ = nullptr;
        }
    }

    // Hall Effect sensor on GPIO5 (not I2C, always try to init)
    hall_ = new HallSensor();
    if (hall_->begin(5, true)) {
        hallAvailable_ = true;
        anyFound = true;
        Serial.println("[SensorManager] Hall Effect sensor on GPIO5");
    } else {
        delete hall_;
        hall_ = nullptr;
    }

    return anyFound;
}

bool SensorManager::scanI2CAddress(uint8_t addr) {
    Wire.beginTransmission(addr);
    return (Wire.endTransmission() == 0);
}

String SensorManager::getAvailableSensors() const {
    String sensors = "";
    if (bme688Available_) sensors += "BME688, ";
    if (adxl345Available_) sensors += "ADXL345, ";
    if (rfidAvailable_) sensors += "RFID, ";
    if (hallAvailable_) sensors += "Hall, ";

    if (sensors.length() > 0) {
        sensors.remove(sensors.length() - 2); // Remove trailing ", "
    } else {
        sensors = "None";
    }
    return sensors;
}

bool SensorManager::executeFunction(const String& functionName, const String& arguments, JsonDocument& result) {
    Serial.printf("[SensorManager] Executing function: %s(%s)\n", functionName.c_str(), arguments.c_str());

    // Parse arguments JSON
    JsonDocument argsDoc;
    DeserializationError error = deserializeJson(argsDoc, arguments);
    if (error && arguments.length() > 0) {
        Serial.printf("[SensorManager] ERROR: Failed to parse arguments: %s\n", error.c_str());
        result["error"] = "Invalid arguments JSON";
        return false;
    }

    // Route to appropriate function
    if (functionName == "read_bme688") {
        String sensor = argsDoc["sensor"] | "temperature";
        return readBME688(sensor, result);
    }
    else if (functionName == "read_accelerometer") {
        String axis = argsDoc["axis"] | "all";
        return readAccelerometer(axis, result);
    }
    else if (functionName == "detect_magnetic_field") {
        return detectMagneticField(result);
    }
    else if (functionName == "read_rfid_card") {
        return readRFIDCard(result);
    }
    else {
        Serial.printf("[SensorManager] ERROR: Unknown function: %s\n", functionName.c_str());
        result["error"] = "Unknown function";
        return false;
    }
}

bool SensorManager::readBME688(const String& sensorType, JsonDocument& result) {
    if (!bme688Available_ || !bme688_) {
        result["error"] = "BME688 sensor not available";
        return false;
    }

    float temp, humidity, pressure, gas;

    if (sensorType == "all") {
        if (bme688_->readAll(temp, humidity, pressure, gas)) {
            result["temperature"] = round(temp * 10) / 10.0;
            result["humidity"] = round(humidity * 10) / 10.0;
            result["pressure"] = round(pressure * 10) / 10.0;
            result["gas_resistance"] = (int)gas;
            return true;
        }
    }
    else if (sensorType == "temperature") {
        if (bme688_->readTemperature(temp)) {
            result["temperature"] = round(temp * 10) / 10.0;
            result["unit"] = "celsius";
            return true;
        }
    }
    else if (sensorType == "humidity") {
        if (bme688_->readHumidity(humidity)) {
            result["humidity"] = round(humidity * 10) / 10.0;
            result["unit"] = "percent";
            return true;
        }
    }
    else if (sensorType == "pressure") {
        if (bme688_->readPressure(pressure)) {
            result["pressure"] = round(pressure * 10) / 10.0;
            result["unit"] = "hPa";
            return true;
        }
    }
    else if (sensorType == "gas") {
        if (bme688_->readGasResistance(gas)) {
            result["gas_resistance"] = (int)gas;
            result["unit"] = "ohms";
            return true;
        }
    }

    result["error"] = "Failed to read BME688 sensor";
    return false;
}

bool SensorManager::readAccelerometer(const String& axis, JsonDocument& result) {
    if (!adxl345Available_ || !adxl345_) {
        result["error"] = "ADXL345 sensor not available";
        return false;
    }

    float x, y, z;
    if (axis == "all") {
        if (adxl345_->readAcceleration(x, y, z)) {
            result["x"] = round(x * 100) / 100.0;
            result["y"] = round(y * 100) / 100.0;
            result["z"] = round(z * 100) / 100.0;
            result["unit"] = "g";
            return true;
        }
    }
    else {
        float value;
        if (adxl345_->readAxis(axis[0], value)) {
            result[axis] = round(value * 100) / 100.0;
            result["unit"] = "g";
            return true;
        }
    }

    result["error"] = "Failed to read ADXL345 sensor";
    return false;
}

bool SensorManager::detectMagneticField(JsonDocument& result) {
    if (!hallAvailable_ || !hall_) {
        result["error"] = "Hall Effect sensor not available";
        return false;
    }

    bool detected = hall_->isMagnetDetected();
    result["magnetic_field_detected"] = detected;
    result["pin"] = hall_->getPin();
    return true;
}

bool SensorManager::readRFIDCard(JsonDocument& result) {
    if (!rfidAvailable_ || !rfid_) {
        result["error"] = "RFID sensor not available";
        return false;
    }

    String uid;
    if (rfid_->getUIDString(uid)) {
        result["card_present"] = true;
        result["uid"] = uid;
        return true;
    } else {
        result["card_present"] = false;
        return true; // Not an error - just no card
    }
}

String SensorManager::readSensorByString(const String& device, const String& parameter) {
    String deviceLower = device;
    String paramLower = parameter;
    deviceLower.toLowerCase();
    paramLower.toLowerCase();

    // BME688 Environmental Sensor
    if (deviceLower == "bme688") {
        if (!bme688Available_ || !bme688_) {
            return "[BME688 not available]";
        }

        float value;
        if (paramLower == "temperature" || paramLower == "temp") {
            if (bme688_->readTemperature(value)) {
                return String(value, 1); // 1 decimal place
            }
        } else if (paramLower == "humidity") {
            if (bme688_->readHumidity(value)) {
                return String(value, 1);
            }
        } else if (paramLower == "pressure") {
            if (bme688_->readPressure(value)) {
                return String(value, 1);
            }
        } else if (paramLower == "gas") {
            if (bme688_->readGasResistance(value)) {
                return String((int)value);
            }
        } else if (paramLower == "all") {
            float temp, humidity, pressure, gas;
            if (bme688_->readAll(temp, humidity, pressure, gas)) {
                return String(temp, 1) + "°C, " +
                       String(humidity, 1) + "%, " +
                       String(pressure, 1) + "hPa";
            }
        }
        return "[BME688 read error]";
    }

    // ADXL345 Accelerometer
    else if (deviceLower == "adxl345" || deviceLower == "accel") {
        if (!adxl345Available_ || !adxl345_) {
            return "[ADXL345 not available]";
        }

        float x, y, z;
        if (paramLower == "x") {
            if (adxl345_->readAxis('x', x)) {
                return String(x, 2); // 2 decimal places for g
            }
        } else if (paramLower == "y") {
            if (adxl345_->readAxis('y', y)) {
                return String(y, 2);
            }
        } else if (paramLower == "z") {
            if (adxl345_->readAxis('z', z)) {
                return String(z, 2);
            }
        } else if (paramLower == "all") {
            if (adxl345_->readAcceleration(x, y, z)) {
                return "X:" + String(x, 2) + "g, Y:" + String(y, 2) + "g, Z:" + String(z, 2) + "g";
            }
        }
        return "[ADXL345 read error]";
    }

    // Hall Effect Sensor
    else if (deviceLower == "hall" || deviceLower == "magnet") {
        if (!hallAvailable_ || !hall_) {
            return "[Hall sensor not available]";
        }

        if (paramLower == "detect" || paramLower == "detected") {
            bool detected = hall_->isMagnetDetected();
            return detected ? "detected" : "not detected";
        }
        return "[Hall read error]";
    }

    // RFID Reader
    else if (deviceLower == "rfid" || deviceLower == "card") {
        if (!rfidAvailable_ || !rfid_) {
            return "[RFID not available]";
        }

        if (paramLower == "uid" || paramLower == "id") {
            String uid;
            if (rfid_->getUIDString(uid)) {
                return uid;
            } else {
                return "no card";
            }
        }
        return "[RFID read error]";
    }

    // Unknown device
    return "[Unknown sensor: " + device + "]";
}

// Accessor methods
BME688Sensor* SensorManager::getBME688() { return bme688_; }
ADXL345Sensor* SensorManager::getADXL345() { return adxl345_; }
RFIDSensor* SensorManager::getRFID() { return rfid_; }
HallSensor* SensorManager::getHall() { return hall_; }
