#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BME680.h>

/**
 * @brief BME688 Environmental Sensor (Temperature, Humidity, Pressure, Gas)
 *
 * Bosch BME688 is a 4-in-1 environmental sensor with gas resistance measurement
 * for air quality estimation. Communicates via I2C.
 */
class BME688Sensor {
public:
    BME688Sensor();
    ~BME688Sensor();

    /**
     * @brief Initialize BME688 sensor
     * @param wire I2C bus instance
     * @param address I2C address (0x76 or 0x77)
     * @return true if initialization successful
     */
    bool begin(TwoWire* wire, uint8_t address = 0x77);

    /**
     * @brief Read temperature in Celsius
     * @param temp Output temperature value
     * @return true if read successful
     */
    bool readTemperature(float& temp);

    /**
     * @brief Read relative humidity in %
     * @param humidity Output humidity value
     * @return true if read successful
     */
    bool readHumidity(float& humidity);

    /**
     * @brief Read atmospheric pressure in hPa
     * @param pressure Output pressure value
     * @return true if read successful
     */
    bool readPressure(float& pressure);

    /**
     * @brief Read gas resistance in ohms (air quality indicator)
     * @param gasResistance Output gas resistance value
     * @return true if read successful
     */
    bool readGasResistance(float& gasResistance);

    /**
     * @brief Read all sensor values at once
     * @return true if read successful
     */
    bool readAll(float& temp, float& humidity, float& pressure, float& gas);

    /**
     * @brief Check if sensor is available
     */
    bool isAvailable() const { return initialized_; }

    /**
     * @brief Get sensor I2C address
     */
    uint8_t getAddress() const { return address_; }

private:
    Adafruit_BME680 bme_;
    uint8_t address_;
    bool initialized_;
    unsigned long lastReadTime_;
    static const unsigned long MIN_READ_INTERVAL_MS = 1000; // 1 second minimum between reads
};
