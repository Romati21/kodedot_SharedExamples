#include "sensor_manager/HallSensor.h"

HallSensor::HallSensor()
    : pin_(-1)
    , initialized_(false)
    , activeLow_(true) {  // Most Hall effect sensors output LOW when magnet detected
}

HallSensor::~HallSensor() {
}

bool HallSensor::begin(int pin, bool pullup) {
    pin_ = pin;

    if (pullup) {
        pinMode(pin_, INPUT_PULLUP);
    } else {
        pinMode(pin_, INPUT);
    }

    initialized_ = true;
    Serial.printf("[Hall] Initialized on GPIO%d\n", pin_);
    return true;
}

bool HallSensor::isMagnetDetected() {
    if (!initialized_) return false;

    int reading = digitalRead(pin_);

    // Active-low: magnet detected when pin is LOW
    return (activeLow_ && reading == LOW) || (!activeLow_ && reading == HIGH);
}

int HallSensor::getRawValue() {
    if (!initialized_) return HIGH;
    return digitalRead(pin_);
}
