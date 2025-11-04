#pragma once

#include <Arduino.h>

/**
 * @brief Hall Effect Magnetic Field Sensor (Digital GPIO)
 *
 * Simple digital Hall effect sensor that outputs LOW when magnet detected,
 * HIGH when no magnet. Connects to any GPIO pin.
 */
class HallSensor {
public:
    HallSensor();
    ~HallSensor();

    /**
     * @brief Initialize Hall effect sensor
     * @param pin GPIO pin number (e.g., GPIO5)
     * @param pullup Enable internal pullup resistor (default: true)
     * @return true if initialization successful
     */
    bool begin(int pin, bool pullup = true);

    /**
     * @brief Check if magnetic field is detected
     * @return true if magnet is present
     */
    bool isMagnetDetected();

    /**
     * @brief Get raw digital reading
     * @return HIGH or LOW
     */
    int getRawValue();

    /**
     * @brief Check if sensor is available
     */
    bool isAvailable() const { return initialized_; }

    /**
     * @brief Get configured pin number
     */
    int getPin() const { return pin_; }

private:
    int pin_;
    bool initialized_;
    bool activeLow_; // True if sensor outputs LOW when magnet detected
};
