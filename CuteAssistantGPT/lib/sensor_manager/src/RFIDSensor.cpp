#include "sensor_manager/RFIDSensor.h"

RFIDSensor::RFIDSensor()
    : wire_(nullptr)
    , address_(0x28)
    , initialized_(false) {
}

RFIDSensor::~RFIDSensor() {
}

bool RFIDSensor::begin(TwoWire* wire, uint8_t address) {
    wire_ = wire;
    address_ = address;

    // Verify device responds
    wire_->beginTransmission(address_);
    if (wire_->endTransmission() != 0) {
        Serial.printf("[RFID] Device not found at 0x%02X\n", address);
        return false;
    }

    initialized_ = true;
    Serial.printf("[RFID] Initialized at 0x%02X\n", address);
    return true;
}

bool RFIDSensor::isCardPresent() {
    if (!initialized_) return false;

    uint8_t present = readRegister(REG_CARD_PRESENT);
    return (present == 0x01);
}

bool RFIDSensor::readCardUID(uint8_t* uid, uint8_t& uidLength) {
    if (!initialized_ || !isCardPresent()) {
        return false;
    }

    // Read UID length
    uidLength = readRegister(REG_UID_LENGTH);
    if (uidLength == 0 || uidLength > 10) {
        Serial.printf("[RFID] Invalid UID length: %d\n", uidLength);
        return false;
    }

    // Read UID bytes
    readRegisters(REG_UID_START, uid, uidLength);

    return true;
}

bool RFIDSensor::getUIDString(String& uidString) {
    uint8_t uid[10];
    uint8_t uidLength;

    if (!readCardUID(uid, uidLength)) {
        return false;
    }

    uidString = "";
    for (uint8_t i = 0; i < uidLength; i++) {
        if (i > 0) uidString += ":";
        if (uid[i] < 0x10) uidString += "0";
        uidString += String(uid[i], HEX);
    }
    uidString.toUpperCase();

    return true;
}

uint8_t RFIDSensor::readRegister(uint8_t reg) {
    if (!wire_) return 0;

    wire_->beginTransmission(address_);
    wire_->write(reg);
    wire_->endTransmission(false);

    wire_->requestFrom(address_, (uint8_t)1);
    if (wire_->available()) {
        return wire_->read();
    }
    return 0;
}

void RFIDSensor::readRegisters(uint8_t reg, uint8_t* buffer, uint8_t length) {
    if (!wire_) return;

    wire_->beginTransmission(address_);
    wire_->write(reg);
    wire_->endTransmission(false);

    wire_->requestFrom(address_, length);
    for (uint8_t i = 0; i < length; i++) {
        if (wire_->available()) {
            buffer[i] = wire_->read();
        } else {
            buffer[i] = 0;
        }
    }
}
