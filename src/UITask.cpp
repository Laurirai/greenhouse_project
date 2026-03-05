#include "UITask.h"
#include <cstdio>

UITask::UITask(QueueHandle_t uiQueue) : uiQueue(uiQueue) {}

void UITask::start() {
    xTaskCreate(taskFunction, "UI", 1024, this, 1, NULL);
}

void UITask::taskFunction(void* param) {
    static_cast<UITask*>(param)->run();
}

void UITask::run() {
    i2c     = std::make_shared<PicoI2C>(1, 400000);
    display = std::make_unique<ssd1306os>(i2c);

    printf("UITask running\n");

    SensorData data = {};  // start with zeros until first reading arrives
    char buf[32];

    while (true) {
        // update data if new reading available, don't block
        if (xQueueReceive(uiQueue, &data, pdMS_TO_TICKS(100))) {
            display->fill(0);

            snprintf(buf, sizeof(buf), "CO2:%2.0fppm", data.co2_ppm);
            display->text(buf, 0, 0);

            snprintf(buf, sizeof(buf), "RH:%4.1f%%", data.rh);
            display->text(buf, 0, 10);

            snprintf(buf, sizeof(buf), "Temp:%1.1fC", data.temp);
            display->text(buf, 0, 20);

            snprintf(buf, sizeof(buf), "Pres:%3.1fPa", data.pressure);
            display->text(buf, 0, 30);

            display->show();
        }

    }
}