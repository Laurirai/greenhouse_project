#ifndef SENSORTASK_H
#define SENSORTASK_H

#include <memory>
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "ModbusClient.h"
#include "ModbusRegister.h"
#include "PicoI2C.h"
#include "pressure_sensor/PressureSensor.h"
#include "task.h"

struct SensorData {
    float co2_ppm;
    float rh;
    float temp;
    float pressure;
};

class SensorTask {
public:
    SensorTask(QueueHandle_t uiQueue, QueueHandle_t controlQueue,
               std::shared_ptr<ModbusClient> modbus, SemaphoreHandle_t modbusMutex,
               std::shared_ptr<PicoI2C> i2c);
    void start();
private:
    static void taskFunction(void* param);
    void run();

    QueueHandle_t uiQueue;
    QueueHandle_t controlQueue;
    SemaphoreHandle_t modbusMutex;
    std::shared_ptr<ModbusClient> modbus;
    std::shared_ptr<PicoI2C> i2c;
    std::unique_ptr<PressureSensor> pressure_sensor;
    std::unique_ptr<ModbusRegister> co2_reg;
    std::unique_ptr<ModbusRegister> rh_reg;
    std::unique_ptr<ModbusRegister> temp_reg;
};

#endif