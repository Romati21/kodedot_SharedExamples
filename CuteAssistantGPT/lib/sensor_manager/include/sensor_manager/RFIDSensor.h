#pragma once

#include <Arduino.h>
#include <Wire.h>

/**
 * @brief RFID 2 UNIT Reader (MFRC522-based I2C module)
 *
 * Reads RFID/NFC card UIDs via I2C interface.
 * M5Stack RFID 2 UNIT uses I2C address 0x28.
 */
class RFIDSensor {
public:
    RFIDSensor();
    ~RFIDSensor();

    /**
     * @brief Initialize RFID reader
     * @param wire I2C bus instance
     * @param address I2C address (default 0x28)
     * @return true if initialization successful
     */
    bool begin(TwoWire* wire, uint8_t address = 0x28);

    /**
     * @brief Check if card is present and read UID
     * @param uid Output buffer for UID (at least 10 bytes)
     * @param uidLength Output UID length (typically 4 or 7 bytes)
     * @return true if card detected and UID read
     */
    bool readCardUID(uint8_t* uid, uint8_t& uidLength);

    /**
     * @brief Get UID as hex string (e.g., "04:A3:2C:BA")
     * @param uidString Output string buffer
     * @return true if card detected
     */
    bool getUIDString(String& uidString);

    /**
     * @brief Check if card is present without reading
     * @return true if card detected
     */
    bool isCardPresent();

    /**
     * @brief Check if sensor is available
     */
    bool isAvailable() const { return initialized_; }

private:
    TwoWire* wire_;
    uint8_t address_;
    bool initialized_;

    // I2C register addresses for M5Stack RFID 2 UNIT
    static const uint8_t REG_CARD_PRESENT = 0x01;
    static const uint8_t REG_UID_LENGTH = 0x02;
    static const uint8_t REG_UID_START = 0x03;

    // Helper methods
    uint8_t readRegister(uint8_t reg);
    void readRegisters(uint8_t reg, uint8_t* buffer, uint8_t length);
};
