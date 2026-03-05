#include "SensorTask.h"
#include <cstdio>

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

    // All three use register 256, different device addresses
    co2_reg  = std::make_unique<ModbusRegister>(modbus, 240, 256); // GMP252
    rh_reg   = std::make_unique<ModbusRegister>(modbus, 241, 256); // HMP60 RH
    temp_reg = std::make_unique<ModbusRegister>(modbus, 241, 257); // HMP60 Temp

    printf("SensorTask running\n");

    while (true) {
        SensorData data;

        data.co2_ppm = co2_reg->read();
        vTaskDelay(pdMS_TO_TICKS(10));
        data.rh   = rh_reg->read()   / 10.0f;
        vTaskDelay(pdMS_TO_TICKS(10));
        data.temp = temp_reg->read() / 10.0f;

        printf("CO2: %.0f ppm | RH: %.1f%% | Temp: %.1f C\n",
               data.co2_ppm, data.rh, data.temp);

        xQueueSend(uiQueue, &data, 0);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}