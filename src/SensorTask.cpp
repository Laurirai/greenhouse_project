#include "SensorTask.h"
#include <cstdio>

SensorTask::SensorTask(QueueHandle_t uiQueue, QueueHandle_t controlQueue,
                       std::shared_ptr<ModbusClient> modbus, SemaphoreHandle_t modbusMutex,
                       std::shared_ptr<PicoI2C> i2c)
    : uiQueue(uiQueue), controlQueue(controlQueue),
      modbus(modbus), modbusMutex(modbusMutex), i2c(i2c) {}

void SensorTask::start() {
    xTaskCreate(taskFunction, "Sensor", 1024, this, 2, NULL);
}

void SensorTask::taskFunction(void* param) {
    static_cast<SensorTask*>(param)->run();
}

void SensorTask::run() {
    pressure_sensor = std::make_unique<PressureSensor>(i2c, 0x40);

    co2_reg  = std::make_unique<ModbusRegister>(modbus, 240, 256);
    rh_reg   = std::make_unique<ModbusRegister>(modbus, 241, 256);
    temp_reg = std::make_unique<ModbusRegister>(modbus, 241, 257);

    printf("SensorTask running\n");

    while (true) {
        SensorData data;

        if (xSemaphoreTake(modbusMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            data.co2_ppm = co2_reg->read();
            vTaskDelay(pdMS_TO_TICKS(10));
            data.rh      = rh_reg->read() / 10.0f;
            vTaskDelay(pdMS_TO_TICKS(10));
            data.temp    = temp_reg->read() / 10.0f;
            xSemaphoreGive(modbusMutex);
        }

        data.pressure = pressure_sensor->read();

        printf("CO2:%.0f RH:%.1f T:%.1f P:%.2f\n",
               data.co2_ppm, data.rh, data.temp, data.pressure);

        xQueueSend(uiQueue, &data, 0);
        xQueueSend(controlQueue, &data, 0);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}