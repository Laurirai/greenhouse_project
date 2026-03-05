#include "PressureSensor.h"
#include "FreeRTOS.h"
#include "task.h"
#include <cstdio>

PressureSensor::PressureSensor(std::shared_ptr<PicoI2C> i2cbus, uint8_t address)
    : i2c(std::move(i2cbus)), addr(address) {
    uint8_t reset_cmd = 0x06;
    i2c->write(addr, &reset_cmd, 1);
    vTaskDelay(pdMS_TO_TICKS(20));
    printf("PressureSensor initialized at 0x%02X\n", addr);
}

int16_t PressureSensor::read_raw() {
    uint8_t command = 0xF1;
    auto write = i2c->write(addr, &command, 1);
    if (write != 1) {
        printf("PressureSensor write failed\n");
        return 0;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
    uint8_t buffer[3];
    auto rv = i2c->read(addr, buffer, 3);
    if (rv == 3) {
        int16_t raw_val = (buffer[0] << 8) | buffer[1];
        return raw_val;
    }
    printf("PressureSensor read failed\n");
    return 0;
}

double PressureSensor::read() {
    return read_raw() / 240.0f;
}