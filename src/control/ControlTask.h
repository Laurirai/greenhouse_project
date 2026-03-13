#ifndef CONTROLTASK_H
#define CONTROLTASK_H

#include <memory>
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "ModbusClient.h"
#include "ModbusRegister.h"
#include "eeprom/eeprom.h"
#include "structs.h"

class ControlTask {
public:
    ControlTask(QueueHandle_t controlQueue, EEPROMManager &eeprom,
                std::shared_ptr<ModbusClient> modbus, SemaphoreHandle_t modbusMutex);
    void start();
private:
    bool co2ValveOpen;
    TickType_t co2ValveOpenedAt;
    TickType_t co2ValveLastOpen;

    void initCO2Valve();
    void openCO2Valve();
    void closeCO2Valve();
    void updateCO2Valve(float co2, uint16_t setpoint);
    static void taskFunction(void* param);
    void run();

    void setFanSpeed(uint16_t percent);

    QueueHandle_t controlQueue;
    EEPROMManager &eeprom;
    SemaphoreHandle_t modbusMutex;
    std::shared_ptr<ModbusClient> modbus;
    std::unique_ptr<ModbusRegister> fan_reg;
};

#endif