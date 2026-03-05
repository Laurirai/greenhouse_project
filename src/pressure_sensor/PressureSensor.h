#ifndef PRESSURESENSOR_H
#define PRESSURESENSOR_H

#include <memory>
#include "PicoI2C.h"

class PressureSensor {
public:
    PressureSensor(std::shared_ptr<PicoI2C> i2cbus, uint8_t address = 0x40);
    double read();
private:
    int16_t read_raw();
    std::shared_ptr<PicoI2C> i2c;
    uint8_t addr;
};

#endif