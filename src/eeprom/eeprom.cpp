#include "eeprom.h"

EEPROMManager::EEPROMManager(i2c_inst_t* i2c, uint8_t sda_pin, uint8_t scl_pin)
    : i2c_(i2c), initialized_(false), sda_pin_(sda_pin), scl_pin_(scl_pin) {}

bool EEPROMManager::resetUsedSpace() {
    if (!initialized_) {
        return false;
    }

    uint32_t magic = EEPROM_MAGIC_NUMBER;
    networkConfig empty_cfg{};
    uint32_t co2_set = 0;

    if (!write(ADDR_MAGIC, magic)) {
        return false;
    }

    if (!write(ADDR_SSID, empty_cfg.ssid)) {
        return false;
    }

    if (!write(ADDR_PASS, empty_cfg.password)) {
        return false;
    }

    if (!write(ADDR_CO2S, co2_set)) {
        return false;
    }

    return true;
}
bool EEPROMManager::writeBytes(uint16_t addr, const uint8_t* data, size_t len) {
    size_t bytes_written = 0;

    while (bytes_written < len) {
        uint16_t current_addr = addr + bytes_written;
        uint16_t page_start = (current_addr / EEPROM_PAGE_SIZE) * EEPROM_PAGE_SIZE;
        uint16_t page_end = page_start + EEPROM_PAGE_SIZE - 1;
        size_t bytes_remaining_in_page = page_end - current_addr + 1;
        size_t bytes_to_write = (len - bytes_written < bytes_remaining_in_page)
                                ? (len - bytes_written)
                                : bytes_remaining_in_page;

        uint8_t buffer[2 + EEPROM_PAGE_SIZE];
        buffer[0] = (current_addr >> 8) & 0xFF;
        buffer[1] = current_addr & 0xFF;
        memcpy(buffer + 2, data + bytes_written, bytes_to_write);

        int result = i2c_write_timeout_us(i2c_, EEPROM_ADDR, buffer, bytes_to_write + 2, false, 100000);
        if (result != (int)(bytes_to_write + 2)) return false;

        sleep_ms(EEPROM_WRITE_TIMEOUT);
        bytes_written += bytes_to_write;
    }

    return true;
}

bool EEPROMManager::readBytes(uint16_t addr, uint8_t* data, size_t len) {
    uint8_t addr_buf[2];
    addr_buf[0] = (addr >> 8) & 0xFF;
    addr_buf[1] = addr & 0xFF;

    int write_result = i2c_write_timeout_us(i2c_, EEPROM_ADDR, addr_buf, 2, true, 100000);
    if (write_result != 2) return false;

    int read_result = i2c_read_timeout_us(i2c_, EEPROM_ADDR, data, len, false, 100000);
    return read_result == (int)len;
}

bool EEPROMManager::init() {
    gpio_set_function(sda_pin_, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin_, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin_);
    gpio_pull_up(scl_pin_);
    i2c_init(i2c_, 100000);

    uint8_t addr_buf[1] = {0};
    int result = i2c_write_timeout_us(i2c_, EEPROM_ADDR, addr_buf, 1, false, 100000);
    if (result == PICO_ERROR_GENERIC || result == PICO_ERROR_TIMEOUT) return false;

    uint8_t test_byte = 0;
    result = i2c_read_timeout_us(i2c_, EEPROM_ADDR, &test_byte, 1, false, 100000);
    if (result != 1) return false;

    uint32_t magic = 0;
    if (!readBytes(ADDR_MAGIC, reinterpret_cast<uint8_t*>(&magic), sizeof(magic))) return false;

    if (magic != EEPROM_MAGIC_NUMBER) {
        magic = EEPROM_MAGIC_NUMBER;
        if (!writeBytes(ADDR_MAGIC, reinterpret_cast<uint8_t*>(&magic), sizeof(magic))) return false;
    }

    initialized_ = true;
    return true;
}

bool EEPROMManager::isInitialized() const {
    return initialized_;
}

bool EEPROMManager::loadNetworkConfig(networkConfig &cfg) {
    memset(&cfg, 0, sizeof(cfg));

    if (!read(ADDR_SSID, cfg.ssid)) return false;
    if (!read(ADDR_PASS, cfg.password)) return false;

    cfg.ssid[sizeof(cfg.ssid) - 1] = '\0';
    cfg.password[sizeof(cfg.password) - 1] = '\0';
    return true;
}

bool EEPROMManager::saveNetworkConfig(const networkConfig &cfg) {
    if (!write(ADDR_SSID, cfg.ssid)) return false;
    if (!write(ADDR_PASS, cfg.password)) return false;
    return true;
}

bool EEPROMManager::loadCO2Setpoint(uint32_t &value) {
    value = 0;
    return read(ADDR_CO2S, value);
}

bool EEPROMManager::saveCO2Setpoint(uint32_t value) {
    return write(ADDR_CO2S, value);
}