#include "SensorTask.h"
#include <cstdio>
#include "pressure_sensor/PressureSensor.h"

SensorTask::SensorTask(QueueHandle_t uiQueue) : uiQueue(uiQueue) {}

void SensorTask::start() {
    xTaskCreate(taskFunction, "Sensor", 1024, this, 2, NULL);
}

void SensorTask::taskFunction(void* param) {
    static_cast<SensorTask*>(param)->run();
}

void SensorTask::run() {
    uart   = std::make_shared<PicoOsUart>(1, 4, 5, 9600, 2);
    modbus = std::make_shared<ModbusClient>(uart);
    i2c    = std::make_shared<PicoI2C>(1, 100000);
    pressure_sensor = std::make_unique<PressureSensor>(i2c, 0x40);

    co2_reg  = std::make_unique<ModbusRegister>(modbus, 240, 256);
    rh_reg   = std::make_unique<ModbusRegister>(modbus, 241, 256);
    temp_reg = std::make_unique<ModbusRegister>(modbus, 241, 257);

    while (true) {
        SensorData data;
        data.co2_ppm  = co2_reg->read();
        vTaskDelay(pdMS_TO_TICKS(10));
        data.rh       = rh_reg->read() / 10.0f;
        vTaskDelay(pdMS_TO_TICKS(10));
        data.temp     = temp_reg->read() / 10.0f;
        vTaskDelay(pdMS_TO_TICKS(10));
        data.pressure = pressure_sensor->read();

        printf("CO2:%.0f RH:%.1f T:%.1f P:%.2f\n",
               data.co2_ppm, data.rh, data.temp, data.pressure);

        xQueueSend(uiQueue, &data, 0);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}