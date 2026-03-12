#include "control/ControlTask.h"
#include <cstdio>

#include "SensorTask.h"
#include "structs.h"

ControlTask::ControlTask(QueueHandle_t controlQueue, EEPROMManager &eeprom,
                         std::shared_ptr<ModbusClient> modbus, SemaphoreHandle_t modbusMutex)
    : controlQueue(controlQueue), eeprom(eeprom),
      modbus(modbus), modbusMutex(modbusMutex) {}

void ControlTask::start() {
    xTaskCreate(taskFunction, "Control", 2048, this, tskIDLE_PRIORITY+2, NULL);
}

void ControlTask::taskFunction(void* param) {
    static_cast<ControlTask*>(param)->run();
}


void ControlTask::setFanSpeed(uint16_t percent) {
    fan_reg->write(percent * 10);
    printf("Fan speed set to %u%%\n", percent);
}

void ControlTask::run() {
    fan_reg = std::make_unique<ModbusRegister>(modbus, 1, 0);

    printf("ControlTask running\n");

    uint16_t last_fan_speed = 255;

    while (true) {
        SensorData data{};

        if (xQueueReceive(controlQueue, &data, pdMS_TO_TICKS(5000)) == pdTRUE) {
            uint16_t fan_speed = calculateFanSpeed(data.co2_ppm, co2setpoint);

            if (fan_speed != last_fan_speed) {
                if (xSemaphoreTake(modbusMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
                    setFanSpeed(fan_speed);
                    xSemaphoreGive(modbusMutex);
                    last_fan_speed = fan_speed;
                } else {
                    printf("ControlTask: Modbus mutex timeout, fan unchanged.\n");
                }
            }

            printf("ControlTask: CO2=%.0f setpoint=%u fan=%u%%\n",
                   data.co2_ppm, co2setpoint, fan_speed);
        }
    }
}