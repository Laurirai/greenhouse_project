#ifndef CONTROLTASK_H
#define CONTROLTASK_H

#include <memory>
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "ModbusClient.h"
#include "ModbusRegister.h"
#include "eeprom/eeprom.h"

class ControlTask {
public:
    ControlTask(QueueHandle_t controlQueue, EEPROMManager &eeprom,
                std::shared_ptr<ModbusClient> modbus, SemaphoreHandle_t modbusMutex);
    void start();
private:
    static void taskFunction(void* param);
    void run();

    uint16_t calculateFanSpeed(float co2, uint32_t setpoint);
    void setFanSpeed(uint16_t percent);

    QueueHandle_t controlQueue;
    EEPROMManager &eeprom;
    SemaphoreHandle_t modbusMutex;
    std::shared_ptr<ModbusClient> modbus;
    std::unique_ptr<ModbusRegister> fan_reg;
};

#endif