#include "UITask.h"
#include "inputs/InputHandler.h"
#include <cstdio>

UITask::UITask(QueueHandle_t uiQueue, QueueHandle_t inputQueue)
    : uiQueue(uiQueue), inputQueue(inputQueue) {}

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

    SensorData data = {};
    InputEvent ev;
    int selected = 0;
    char buf[32];
    bool needs_redraw = true;

    while (true) {
        // check for input events, non-blocking
        if (xQueueReceive(inputQueue, &ev, 0) == pdTRUE) {
            if (ev == ENC_DOWN && selected < 1) selected++;
            if (ev == ENC_UP   && selected > 0) selected--;
            needs_redraw = true;
        }

        // grab latest sensor data if available
        if (xQueueReceive(uiQueue, &data, 0) == pdTRUE) {
            needs_redraw = true;
        }

        if (needs_redraw) {
            display->fill(0);

            // title
            snprintf(buf, sizeof(buf), "Auto greenhouse");
            display->text(buf, 0, 0);

            // sensor data on left
            snprintf(buf, sizeof(buf), "CO2:%1.1fppm", data.co2_ppm);
            display->text(buf, 0, 15);

            snprintf(buf, sizeof(buf), "RH: %1.1f%%", data.rh);
            display->text(buf, 0, 25);

            snprintf(buf, sizeof(buf), "T:  %1.1fC", data.temp);
            display->text(buf, 0, 35);

            snprintf(buf, sizeof(buf), "P:  %1.1fPa", data.pressure);
            display->text(buf, 0, 45);

            // menu on right
            snprintf(buf, sizeof(buf), "SET");
            display->text(buf, 95, 30);

            snprintf(buf, sizeof(buf), "%sval", selected == 0 ? "*" : " ");
            display->text(buf, 90, 40);

            snprintf(buf, sizeof(buf), "%sId", selected == 1 ? "*" : " ");
            display->text(buf, 90, 50);

            display->show();
            needs_redraw = false;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}