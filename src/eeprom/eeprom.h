#ifndef EEPROMMANAGER_H
#define EEPROMMANAGER_H

#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include <cstring>
#include <cstdint>
#include "structs.h"

#define EEPROM_ADDR 0x50
#define EEPROM_PAGE_SIZE 32
#define EEPROM_WRITE_TIMEOUT 5    // ms to wait after write

#define ADDR_MAGIC 0x0000
#define ADDR_SSID  0x0004
#define ADDR_PASS  0x0024
#define ADDR_CO2S  0x0064
#define EEPROM_MAGIC_NUMBER 0xDEADBEEF

class EEPROMManager {
private:
    i2c_inst_t* i2c_;
    bool initialized_;
    uint8_t sda_pin_;
    uint8_t scl_pin_;

    bool writeBytes(uint16_t addr, const uint8_t* data, size_t len);
    bool readBytes(uint16_t addr, uint8_t* data, size_t len);

public:
    EEPROMManager(i2c_inst_t* i2c, uint8_t sda_pin, uint8_t scl_pin);

    bool init();
    bool isInitialized() const;
    bool resetUsedSpace();
    template<typename T>
    bool write(uint16_t addr, const T& data) {
        if (!initialized_) return false;
        return writeBytes(addr, reinterpret_cast<const uint8_t*>(&data), sizeof(T));
    }

    template<typename T>
    bool read(uint16_t addr, T& data) {
        if (!initialized_) return false;
        return readBytes(addr, reinterpret_cast<uint8_t*>(&data), sizeof(T));
    }

    bool loadNetworkConfig(networkConfig &cfg);
    bool saveNetworkConfig(const networkConfig &cfg);

    bool loadCO2Setpoint(uint32_t &value);
    bool saveCO2Setpoint(uint32_t value);
};

#endif // EEPROMMANAGER_H