#ifndef SENSORTASK_H
#define SENSORTASK_H

#include <memory>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "ModbusClient.h"
#include "ModbusRegister.h"
#include "PicoOsUart.h"

struct SensorData {
    float co2_ppm;
    float rh;
    float temp;
};

class SensorTask {
public:
    explicit SensorTask(QueueHandle_t uiQueue);
    void start();
private:
    static void taskFunction(void* param);
    void run();

    QueueHandle_t uiQueue;
    std::shared_ptr<PicoOsUart> uart;
    std::shared_ptr<ModbusClient> modbus;
    std::unique_ptr<ModbusRegister> co2_reg;
    std::unique_ptr<ModbusRegister> rh_reg;
    std::unique_ptr<ModbusRegister> temp_reg;
};

#endif