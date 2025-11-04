#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>

// Forward declarations for sensor classes
class BME688Sensor;
class ADXL345Sensor;
class RFIDSensor;
class HallSensor;

/**
 * @brief Central manager for all sensors with Function Calling support
 *
 * Coordinates I2C bus initialization, sensor detection, and provides
 * unified interface for OpenAI Function Calling API.
 */
class SensorManager {
public:
    SensorManager();
    ~SensorManager();

    /**
     * @brief Initialize I2C bus and detect connected sensors
     * @param sda I2C SDA pin (default: GPIO11 for CoreS3-Lite)
     * @param scl I2C SCL pin (default: GPIO12 for CoreS3-Lite)
     * @return true if at least one sensor detected
     */
    bool init(int sda = 11, int scl = 12);

    /**
     * @brief Execute OpenAI function call and return JSON result
     * @param functionName Name of the function (e.g., "read_bme688")
     * @param arguments JSON arguments string (e.g., '{"sensor":"temperature"}')
     * @param result Output JSON document with function result
     * @return true if function executed successfully
     */
    bool executeFunction(const String& functionName, const String& arguments, JsonDocument& result);

    /**
     * @brief Get list of available sensors
     * @return Comma-separated string of detected sensors
     */
    String getAvailableSensors() const;

    /**
     * @brief Read sensor value by string query (for placeholder replacement)
     * @param device Device name (e.g., "bme688", "adxl345", "hall", "rfid")
     * @param parameter Parameter name (e.g., "temperature", "x", "detect", "uid")
     * @return String representation of sensor value (e.g., "23.5", "0.82", "true", "04:A3:2C:BA")
     */
    String readSensorByString(const String& device, const String& parameter);

    // Individual sensor access (returns nullptr if not available)
    BME688Sensor* getBME688();
    ADXL345Sensor* getADXL345();
    RFIDSensor* getRFID();
    HallSensor* getHall();

    // Function implementations (called via executeFunction)
    bool readBME688(const String& sensorType, JsonDocument& result);
    bool readAccelerometer(const String& axis, JsonDocument& result);
    bool detectMagneticField(JsonDocument& result);
    bool readRFIDCard(JsonDocument& result);

private:
    // Sensor instances (created only if detected)
    BME688Sensor* bme688_;
    ADXL345Sensor* adxl345_;
    RFIDSensor* rfid_;
    HallSensor* hall_;

    // I2C pins
    int sdaPin_;
    int sclPin_;

    // Detection flags
    bool bme688Available_;
    bool adxl345Available_;
    bool rfidAvailable_;
    bool hallAvailable_;

    // Helper methods
    bool detectSensors();
    bool scanI2CAddress(uint8_t addr);
};
