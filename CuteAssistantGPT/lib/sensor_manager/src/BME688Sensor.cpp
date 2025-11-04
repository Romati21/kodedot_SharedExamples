#include "sensor_manager/BME688Sensor.h"

BME688Sensor::BME688Sensor()
    : address_(0x77)
    , initialized_(false)
    , lastReadTime_(0) {
}

BME688Sensor::~BME688Sensor() {
}

bool BME688Sensor::begin(TwoWire* wire, uint8_t address) {
    address_ = address;

    if (!bme_.begin(address, wire)) {
        Serial.printf("[BME688] Failed to initialize at 0x%02X\n", address);
        return false;
    }

    // Set up oversampling and filter initialization
    bme_.setTemperatureOversampling(BME680_OS_8X);
    bme_.setHumidityOversampling(BME680_OS_2X);
    bme_.setPressureOversampling(BME680_OS_4X);
    bme_.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme_.setGasHeater(320, 150); // 320°C for 150 ms for gas measurement

    initialized_ = true;
    Serial.printf("[BME688] Initialized successfully at 0x%02X\n", address);
    return true;
}

bool BME688Sensor::readTemperature(float& temp) {
    if (!initialized_) return false;

    // Respect minimum read interval
    unsigned long now = millis();
    if (now - lastReadTime_ < MIN_READ_INTERVAL_MS) {
        delay(MIN_READ_INTERVAL_MS - (now - lastReadTime_));
    }

    if (!bme_.performReading()) {
        Serial.println("[BME688] ERROR: Failed to perform reading");
        return false;
    }

    temp = bme_.temperature;
    lastReadTime_ = millis();
    return true;
}

bool BME688Sensor::readHumidity(float& humidity) {
    if (!initialized_) return false;

    unsigned long now = millis();
    if (now - lastReadTime_ < MIN_READ_INTERVAL_MS) {
        delay(MIN_READ_INTERVAL_MS - (now - lastReadTime_));
    }

    if (!bme_.performReading()) {
        Serial.println("[BME688] ERROR: Failed to perform reading");
        return false;
    }

    humidity = bme_.humidity;
    lastReadTime_ = millis();
    return true;
}

bool BME688Sensor::readPressure(float& pressure) {
    if (!initialized_) return false;

    unsigned long now = millis();
    if (now - lastReadTime_ < MIN_READ_INTERVAL_MS) {
        delay(MIN_READ_INTERVAL_MS - (now - lastReadTime_));
    }

    if (!bme_.performReading()) {
        Serial.println("[BME688] ERROR: Failed to perform reading");
        return false;
    }

    pressure = bme_.pressure / 100.0; // Convert Pa to hPa
    lastReadTime_ = millis();
    return true;
}

bool BME688Sensor::readGasResistance(float& gasResistance) {
    if (!initialized_) return false;

    unsigned long now = millis();
    if (now - lastReadTime_ < MIN_READ_INTERVAL_MS) {
        delay(MIN_READ_INTERVAL_MS - (now - lastReadTime_));
    }

    if (!bme_.performReading()) {
        Serial.println("[BME688] ERROR: Failed to perform reading");
        return false;
    }

    gasResistance = bme_.gas_resistance;
    lastReadTime_ = millis();
    return true;
}

bool BME688Sensor::readAll(float& temp, float& humidity, float& pressure, float& gas) {
    if (!initialized_) return false;

    unsigned long now = millis();
    if (now - lastReadTime_ < MIN_READ_INTERVAL_MS) {
        delay(MIN_READ_INTERVAL_MS - (now - lastReadTime_));
    }

    if (!bme_.performReading()) {
        Serial.println("[BME688] ERROR: Failed to perform reading");
        return false;
    }

    temp = bme_.temperature;
    humidity = bme_.humidity;
    pressure = bme_.pressure / 100.0; // Convert Pa to hPa
    gas = bme_.gas_resistance;

    lastReadTime_ = millis();
    return true;
}
