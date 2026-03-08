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
    char buf[32];
    bool needs_redraw = true;

    // main screen state
    int main_selected = 0;  // 0 = val, 1 = Id

    // value screen state
    int val_selected = 0;   // 0 = CO2, 1 = RH, 2 = Temp
    float co2_setpoint  = 800.0f;
    float rh_setpoint   = 50.0f;
    float temp_setpoint = 20.0f;

    enum Screen { MAIN, VALUE_SET } current_screen = MAIN;

    while (true) {
        // grab latest sensor data
        if (xQueueReceive(uiQueue, &data, 0) == pdTRUE) {
            needs_redraw = true;
        }

        // handle input
        if (xQueueReceive(inputQueue, &ev, 0) == pdTRUE) {
            needs_redraw = true;

            if (current_screen == MAIN) {
                if (ev == ENC_DOWN && main_selected < 1) main_selected++;
                if (ev == ENC_UP   && main_selected > 0) main_selected--;
                if (ev == ENC_PRESS && main_selected == 0) {
                    current_screen = VALUE_SET;
                    val_selected = 0;
                }
            }
            else if (current_screen == VALUE_SET) {
                if (ev == ENC_DOWN && val_selected < 2) val_selected++;
                if (ev == ENC_UP   && val_selected > 0) val_selected--;

                // SW0 increases, SW2 decreases selected value
                if (ev == BTN_RIGHT || ev == BTN_LEFT) {
                    float delta = (ev == BTN_RIGHT) ? 1.0f : -1.0f;
                    if (val_selected == 0) co2_setpoint  += delta * 10.0f;
                    if (val_selected == 1) rh_setpoint   += delta;
                    if (val_selected == 2) temp_setpoint += delta * 0.5f;
                }

                // encoder press confirms and goes back
                if (ev == BTN_BACK || ev == ENC_PRESS) {
                    current_screen = MAIN;
                }
            }
        }

        if (needs_redraw) {
            display->fill(0);

            if (current_screen == MAIN) {
                snprintf(buf, sizeof(buf), "Auto greenhouse");
                display->text(buf, 0, 0);

                snprintf(buf, sizeof(buf), "CO2:%1.1fppm", data.co2_ppm);
                display->text(buf, 0, 15);

                snprintf(buf, sizeof(buf), "RH: %1.1f%%", data.rh);
                display->text(buf, 0, 25);

                snprintf(buf, sizeof(buf), "T:  %1.1fC", data.temp);
                display->text(buf, 0, 35);

                snprintf(buf, sizeof(buf), "P:  %1.1fPa", data.pressure);
                display->text(buf, 0, 45);

                snprintf(buf, sizeof(buf), "SET");
                display->text(buf, 95, 30);

                snprintf(buf, sizeof(buf), "%sval", main_selected == 0 ? "*" : " ");
                display->text(buf, 90, 40);

                snprintf(buf, sizeof(buf), "%sId", main_selected == 1 ? "*" : " ");
                display->text(buf, 90, 50);
            }
            else if (current_screen == VALUE_SET) {
                snprintf(buf, sizeof(buf), "Set the value of:");
                display->text(buf, 0, 0);

                snprintf(buf, sizeof(buf), "%sCO2  <%.0f>",  val_selected == 0 ? "*" : " ", co2_setpoint);
                display->text(buf, 0, 20);

                snprintf(buf, sizeof(buf), "%sRH   <%.1f>",  val_selected == 1 ? "*" : " ", rh_setpoint);
                display->text(buf, 0, 32);

                snprintf(buf, sizeof(buf), "%sTemp <%.1f>",  val_selected == 2 ? "*" : " ", temp_setpoint);
                display->text(buf, 0, 44);

                snprintf(buf, sizeof(buf), "BACK=encoder/SW1");
                display->text(buf, 0, 56);
            }

            display->show();
            needs_redraw = false;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}