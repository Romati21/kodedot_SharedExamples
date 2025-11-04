#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADXL345_U.h>

/**
 * @brief ADXL345 3-Axis Digital Accelerometer
 *
 * Measures acceleration in 3 axes, supports tilt/tap detection.
 * Communicates via I2C.
 */
class ADXL345Sensor {
public:
    ADXL345Sensor();
    ~ADXL345Sensor();

    /**
     * @brief Initialize ADXL345 sensor
     * @param wire I2C bus instance
     * @param deviceID Unique sensor ID for Adafruit Unified Sensor
     * @return true if initialization successful
     */
    bool begin(TwoWire* wire, int32_t deviceID = 12345);

    /**
     * @brief Read acceleration on all 3 axes
     * @param x X-axis acceleration (g)
     * @param y Y-axis acceleration (g)
     * @param z Z-axis acceleration (g)
     * @return true if read successful
     */
    bool readAcceleration(float& x, float& y, float& z);

    /**
     * @brief Read acceleration on single axis
     * @param axis 'x', 'y', or 'z'
     * @param value Output acceleration value (g)
     * @return true if read successful
     */
    bool readAxis(char axis, float& value);

    /**
     * @brief Calculate tilt angle in degrees
     * @param pitch Output pitch angle (rotation around Y-axis)
     * @param roll Output roll angle (rotation around X-axis)
     * @return true if calculation successful
     */
    bool calculateTilt(float& pitch, float& roll);

    /**
     * @brief Check if sensor is available
     */
    bool isAvailable() const { return initialized_; }

private:
    Adafruit_ADXL345_Unified accel_;
    bool initialized_;
    sensors_event_t lastEvent_;
};
