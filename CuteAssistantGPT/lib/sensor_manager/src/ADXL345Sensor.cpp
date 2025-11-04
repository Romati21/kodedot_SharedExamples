#include "sensor_manager/ADXL345Sensor.h"
#include <math.h>

ADXL345Sensor::ADXL345Sensor()
    : accel_(12345)
    , initialized_(false) {
}

ADXL345Sensor::~ADXL345Sensor() {
}

bool ADXL345Sensor::begin(TwoWire* wire, int32_t deviceID) {
    if (!accel_.begin()) {
        Serial.println("[ADXL345] Failed to initialize");
        return false;
    }

    // Set range to ±16g for maximum sensitivity
    accel_.setRange(ADXL345_RANGE_16_G);

    initialized_ = true;
    Serial.println("[ADXL345] Initialized successfully");
    return true;
}

bool ADXL345Sensor::readAcceleration(float& x, float& y, float& z) {
    if (!initialized_) return false;

    accel_.getEvent(&lastEvent_);

    x = lastEvent_.acceleration.x;
    y = lastEvent_.acceleration.y;
    z = lastEvent_.acceleration.z;

    return true;
}

bool ADXL345Sensor::readAxis(char axis, float& value) {
    if (!initialized_) return false;

    float x, y, z;
    if (!readAcceleration(x, y, z)) {
        return false;
    }

    switch (tolower(axis)) {
        case 'x': value = x; break;
        case 'y': value = y; break;
        case 'z': value = z; break;
        default:
            Serial.printf("[ADXL345] ERROR: Invalid axis '%c'\n", axis);
            return false;
    }

    return true;
}

bool ADXL345Sensor::calculateTilt(float& pitch, float& roll) {
    if (!initialized_) return false;

    float x, y, z;
    if (!readAcceleration(x, y, z)) {
        return false;
    }

    // Calculate pitch and roll in degrees
    // Pitch: rotation around Y-axis
    // Roll: rotation around X-axis
    pitch = atan2(x, sqrt(y * y + z * z)) * 180.0 / M_PI;
    roll = atan2(y, sqrt(x * x + z * z)) * 180.0 / M_PI;

    return true;
}
