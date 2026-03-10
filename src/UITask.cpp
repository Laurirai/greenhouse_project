#include "UITask.h"
#include "inputs/InputHandler.h"
#include <cstdio>

UITask::UITask(QueueHandle_t uiQueue, QueueHandle_t inputQueue, EEPROMManager &eeprom_)
    : uiQueue(uiQueue), inputQueue(inputQueue), eeprom(eeprom_) {}

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

    int main_selected = 0;
    if (eeprom.loadCO2Setpoint(co2_setpoint)) {
        printf("Loaded co2 from eeprom with value: %u\n");
        if (co2_setpoint >= MIN_CO2_SET && co2_setpoint <= MAX_CO2_SET) {
            printf("Read saved CO2 setpoint from EEPROM: %u\n", co2_setpoint);
        } else {
            printf("Trash CO2 setpoint in EEPROM, setting to default of %u\n", DEFAULT_CO2_SET);
            co2_setpoint = DEFAULT_CO2_SET;
        }
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
                // encoder turns adjust co2 target
                if (ev == ENC_DOWN) co2_setpoint += 10;
                if (ev == ENC_UP)   co2_setpoint -= 10;

                // clamp to reasonable range
                if (co2_setpoint < MIN_CO2_SET)    co2_setpoint = MIN_CO2_SET;
                if (co2_setpoint > MAX_CO2_SET)    co2_setpoint = MAX_CO2_SET;

                // encoder press saves and goes back
                if (ev == ENC_PRESS) {
                    printf("CO2 target saved: %.0hu ppm\n", co2_setpoint);
                    // EEPROM write will go here
                    eeprom.saveCO2Setpoint(co2_setpoint);
                    current_screen = MAIN;
                }

                // back button discards and goes back
                if (ev == BTN_BACK) {
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

                snprintf(buf, sizeof(buf), "%.0lu ppm", co2_setpoint);
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