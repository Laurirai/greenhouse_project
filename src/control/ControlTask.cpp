#include "control/ControlTask.h"
#include <cstdio>

#include "SensorTask.h"
#include "structs.h"
#include "hardware/gpio.h"

#define CO2_VALVE_PIN 27

const TickType_t CO2_VALVE_OPEN_TIME = pdMS_TO_TICKS(1000);
const TickType_t CO2_VALVE_INTERVAL = pdMS_TO_TICKS(30000);

ControlTask::ControlTask(QueueHandle_t controlQueue, EEPROMManager &eeprom,
                         std::shared_ptr<ModbusClient> modbus, SemaphoreHandle_t modbusMutex)
    : controlQueue(controlQueue), eeprom(eeprom),
      modbus(modbus), modbusMutex(modbusMutex),
      co2ValveOpen(false), co2ValveOpenedAt(0), co2ValveLastOpen(0) {}

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

void ControlTask::initCO2Valve() {
    gpio_init(CO2_VALVE_PIN);
    gpio_set_dir(CO2_VALVE_PIN, GPIO_OUT);
    gpio_put(CO2_VALVE_PIN, 0);

    co2ValveOpen = false;
    co2ValveOpenedAt = 0;
    co2ValveLastOpen = xTaskGetTickCount() - CO2_VALVE_INTERVAL;
}

void ControlTask::openCO2Valve() {
    gpio_put(CO2_VALVE_PIN, 1);
    co2ValveOpen = true;
    co2ValveOpenedAt = xTaskGetTickCount();
    co2ValveLastOpen = co2ValveOpenedAt;

    printf("CO2 valve opened\n");
}

void ControlTask::closeCO2Valve() {
    gpio_put(CO2_VALVE_PIN, 0);
    co2ValveOpen = false;

    printf("CO2 valve closed\n");
}

void ControlTask::updateCO2Valve(float co2, uint16_t setpoint) {
    TickType_t now = xTaskGetTickCount();

    if (co2ValveOpen) {
        if ((now - co2ValveOpenedAt) >= CO2_VALVE_OPEN_TIME) {
            closeCO2Valve();
        }
        return;
    }

    if ((now - co2ValveLastOpen) < CO2_VALVE_INTERVAL) {
        return;
    }

    if (co2 < setpoint - 20) {
        openCO2Valve();
    }
}

void ControlTask::run() {
    fan_reg = std::make_unique<ModbusRegister>(modbus, 1, 0);

    initCO2Valve();

    printf("ControlTask running\n");

    uint16_t last_fan_speed = 255;
    SensorData data{};
    bool hasData = false;

    while (true) {
        SensorData newData{};

        if (xQueueReceive(controlQueue, &newData, pdMS_TO_TICKS(100)) == pdTRUE) {
            data = newData;
            hasData = true;

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

        if (hasData) {
            updateCO2Valve(data.co2_ppm, co2setpoint);
        }
    }
}