#include "UITask.h"
#include "inputs/InputHandler.h"
#include <cstdio>

UITask::UITask(QueueHandle_t uiQueue, QueueHandle_t inputQueue,
               EEPROMManager &eeprom, std::shared_ptr<PicoI2C> i2c)
    : uiQueue(uiQueue), inputQueue(inputQueue), eeprom(eeprom), i2c(i2c) {}

void UITask::start() {
    xTaskCreate(taskFunction, "UI", 1024, this, 1, NULL);
}

void UITask::taskFunction(void* param) {
    static_cast<UITask*>(param)->run();
}

void UITask::run() {
    display = std::make_unique<ssd1306os>(i2c);

    printf("UITask running\n");

    SensorData data = {};
    InputEvent ev;
    char buf[32];
    bool needs_redraw = true;

    int main_selected = 0;
    uint32_t co2_setpoint = DEFAULT_CO2_SET;

    if (eeprom.loadCO2Setpoint(co2_setpoint)) {
        printf("Loaded CO2 setpoint from EEPROM: %u\n", co2_setpoint);
        if (co2_setpoint < MIN_CO2_SET || co2_setpoint > MAX_CO2_SET) {
            printf("Invalid value, resetting to default: %u\n", DEFAULT_CO2_SET);
            co2_setpoint = DEFAULT_CO2_SET;
        }
    } else {
        printf("Failed to load CO2 setpoint, using default: %u\n", DEFAULT_CO2_SET);
        co2_setpoint = DEFAULT_CO2_SET;
    }

    enum Screen { MAIN, VALUE_SET } current_screen = MAIN;

    while (true) {
        if (xQueueReceive(uiQueue, &data, 0) == pdTRUE) {
            needs_redraw = true;
        }

        if (xQueueReceive(inputQueue, &ev, 0) == pdTRUE) {
            needs_redraw = true;

            if (current_screen == MAIN) {
                if (ev == ENC_DOWN && main_selected < 1) main_selected++;
                if (ev == ENC_UP   && main_selected > 0) main_selected--;
                if (ev == ENC_PRESS && main_selected == 0) {
                    current_screen = VALUE_SET;
                }
            }
            else if (current_screen == VALUE_SET) {
                if (ev == ENC_DOWN) {
                    if (co2_setpoint + 10 <= MAX_CO2_SET)
                        co2_setpoint += 10;
                    else
                        co2_setpoint = MAX_CO2_SET;
                }
                if (ev == ENC_UP) {
                    if (co2_setpoint >= MIN_CO2_SET + 10)
                        co2_setpoint -= 10;
                    else
                        co2_setpoint = MIN_CO2_SET;
                }

                if (ev == ENC_PRESS) {
                    eeprom.saveCO2Setpoint(co2_setpoint);
                    printf("CO2 setpoint saved: %u ppm\n", co2_setpoint);
                    current_screen = MAIN;
                }

                if (ev == BTN_BACK) {
                    // reload from eeprom so unsaved changes are discarded
                    eeprom.loadCO2Setpoint(co2_setpoint);
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
                snprintf(buf, sizeof(buf), "CO2 target:");
                display->text(buf, 0, 0);

                snprintf(buf, sizeof(buf), "%u ppm", co2_setpoint);
                display->text(buf, 20, 25);

                snprintf(buf, sizeof(buf), "ENC=adjust");
                display->text(buf, 0, 45);

                snprintf(buf, sizeof(buf), "PRESS=save SW1=back");
                display->text(buf, 0, 55);
            }

            display->show();
            needs_redraw = false;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}