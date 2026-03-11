#include "control/ControlTask.h"
#include <cstdio>

ControlTask::ControlTask(QueueHandle_t controlQueue, EEPROMManager &eeprom,
                         std::shared_ptr<ModbusClient> modbus, SemaphoreHandle_t modbusMutex)
    : controlQueue(controlQueue), eeprom(eeprom),
      modbus(modbus), modbusMutex(modbusMutex) {}

void ControlTask::start() {
    xTaskCreate(taskFunction, "Control", 1024, this, 2, NULL);
}

void ControlTask::taskFunction(void* param) {
    static_cast<ControlTask*>(param)->run();
}

uint16_t ControlTask::calculateFanSpeed(float co2, uint32_t setpoint) {
    float diff = co2 - (float)setpoint;

    if (diff <= 0)   return 0;
    if (diff < 400)  return 20;
    if (diff < 800)  return 40;
    if (diff < 1200) return 60;
    if (diff < 1600) return 80;
    return 100;
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
        SensorData data;

        if (xQueueReceive(controlQueue, &data, pdMS_TO_TICKS(5000)) == pdTRUE) {
            uint32_t setpoint = DEFAULT_CO2_SET;

            if (EEPROM_ENABLED) {
                uint32_t saved = 0;
                if (eeprom.loadCO2Setpoint(saved)) {
                    if (saved >= MIN_CO2_SET && saved <= MAX_CO2_SET)
                        setpoint = saved;
                }
            }

            uint16_t fan_speed = calculateFanSpeed(data.co2_ppm, setpoint);

            if (fan_speed != last_fan_speed) {
                if (xSemaphoreTake(modbusMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
                    setFanSpeed(fan_speed);
                    xSemaphoreGive(modbusMutex);
                }
                last_fan_speed = fan_speed;
            }

            printf("ControlTask: CO2=%.0f setpoint=%u fan=%u%%\n",
                   data.co2_ppm, setpoint, fan_speed);
        }
    }
}