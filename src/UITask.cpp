#include "UITask.h"
#include "inputs/InputHandler.h"
#include <cstdio>
#include <cstring>

static const char CHAR_LIST[] =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%&*()-_ .";
static const int  CHAR_COUNT  = sizeof(CHAR_LIST) - 1;

UITask::UITask(QueueHandle_t uiQueue, QueueHandle_t inputQueue,
               EEPROMManager &eeprom, std::shared_ptr<PicoI2C> i2c,
               QueueHandle_t networkQueue)
    : uiQueue(uiQueue), inputQueue(inputQueue),
      eeprom(eeprom), i2c(i2c), networkQueue(networkQueue) {}

void UITask::start() {
    xTaskCreate(taskFunction, "UI", 4096, this, tskIDLE_PRIORITY+2, NULL);
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
    uint32_t &co2_setpoint = co2setpoint;

    if (EEPROM_ENABLED) {
        uint32_t stored = DEFAULT_CO2_SET;
        if (eeprom.loadCO2Setpoint(stored) &&
            stored >= MIN_CO2_SET &&
            stored <= MAX_CO2_SET) {
            co2_setpoint = stored;
            printf("Loaded CO2 setpoint from EEPROM: %u\n", co2_setpoint);
        } else {
            co2_setpoint = DEFAULT_CO2_SET;
        }
    } else {
        co2_setpoint = DEFAULT_CO2_SET;
    }

    int id_selected = 0;

    int  char_index = 0;
    char ssid_input[32] = {};
    char pass_input[64] = {};
    int  ssid_len = 0;
    int  pass_len = 0;

    enum Screen {
        MAIN,
        VALUE_SET,
        ID_SCREEN,
        SET_NAME,
        SET_PASS
    } current_screen = MAIN;

    while (true) {
        if (xQueueReceive(uiQueue, &data, 0) == pdTRUE) {
            needs_redraw = true;
        }

        if (xQueueReceive(inputQueue, &ev, 0) == pdTRUE) {
            needs_redraw = true;

            if (current_screen == MAIN) {
                if (ev == ENC_DOWN && main_selected < 1) main_selected++;
                if (ev == ENC_UP   && main_selected > 0) main_selected--;
                if (ev == ENC_PRESS) {
                    if (main_selected == 0) current_screen = VALUE_SET;
                    if (main_selected == 1) {
                        id_selected = 0;
                        current_screen = ID_SCREEN;
                    }
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
                        if (eeprom.saveCO2Setpoint(co2_setpoint)) {
                            printf("CO2 setpoint saved: %u ppm\n", co2_setpoint);
                        } else {
                            printf("Failed to save CO2 setpoint.\n");
                        }
                    current_screen = MAIN;
                }
                if (ev == BTN_BACK) {
                        uint32_t stored = DEFAULT_CO2_SET;
                        if (eeprom.loadCO2Setpoint(stored) &&
                            stored >= MIN_CO2_SET &&
                            stored <= MAX_CO2_SET) {
                            co2_setpoint = stored;
                        } else {
                            co2_setpoint = DEFAULT_CO2_SET;
                        }

                    current_screen = MAIN;
                }
            }

            else if (current_screen == ID_SCREEN) {
                if (ev == ENC_DOWN && id_selected < 2) id_selected++;
                if (ev == ENC_UP   && id_selected > 0) id_selected--;
                if (ev == ENC_PRESS) {
                    char_index = 0;
                    if (id_selected == 0) {
                        memset(ssid_input, 0, sizeof(ssid_input));
                        ssid_len = 0;
                        current_screen = SET_NAME;
                    } else if (id_selected == 1) {
                        memset(pass_input, 0, sizeof(pass_input));
                        pass_len = 0;
                        current_screen = SET_PASS;
                    } else if (id_selected == 2) {
                        if (ssid_len >= 2 && pass_len >= 8) {
                            message msg{};
                            msg.type = NETWORK_CONFIG;
                            strncpy(msg.network_config.ssid, ssid_input, sizeof(msg.network_config.ssid) - 1);
                            strncpy(msg.network_config.password, pass_input, sizeof(msg.network_config.password) - 1);
                            xQueueSend(networkQueue, &msg, 0);
                            printf("Network config sent: %s\n", ssid_input);
                            current_screen = MAIN;
                        } else {
                            printf("SSID or password too short, not sending\n");
                        }
                    }
                }
                if (ev == BTN_BACK || ev == BTN_RIGHT) {
                    current_screen = MAIN;
                }
            }

            else if (current_screen == SET_NAME) {
                if (ev == ENC_DOWN) char_index = (char_index + 1) % CHAR_COUNT;
                if (ev == ENC_UP)   char_index = (char_index - 1 + CHAR_COUNT) % CHAR_COUNT;
                if (ev == ENC_PRESS && ssid_len < 31) {
                    ssid_input[ssid_len++] = CHAR_LIST[char_index];
                    ssid_input[ssid_len] = '\0';
                }
                if (ev == BTN_BACK && ssid_len > 0) {
                    ssid_input[--ssid_len] = '\0';
                }
                if (ev == BTN_LEFT) {
                    current_screen = ID_SCREEN;
                }
                if (ev == BTN_RIGHT) {
                    memset(ssid_input, 0, sizeof(ssid_input));
                    ssid_len = 0;
                    current_screen = ID_SCREEN;
                }
            }

            else if (current_screen == SET_PASS) {
                if (ev == ENC_DOWN) char_index = (char_index + 1) % CHAR_COUNT;
                if (ev == ENC_UP)   char_index = (char_index - 1 + CHAR_COUNT) % CHAR_COUNT;
                if (ev == ENC_PRESS && pass_len < 63) {
                    pass_input[pass_len++] = CHAR_LIST[char_index];
                    pass_input[pass_len] = '\0';
                }
                if (ev == BTN_BACK && pass_len > 0) {
                    pass_input[--pass_len] = '\0';
                }
                if (ev == BTN_LEFT) {
                    current_screen = ID_SCREEN;
                }
                if (ev == BTN_RIGHT) {
                    memset(pass_input, 0, sizeof(pass_input));
                    pass_len = 0;
                    current_screen = ID_SCREEN;
                }
            }
        }

        if (needs_redraw) {
            display->fill(0);

            if (current_screen == MAIN) {
                uint16_t display_fan = 0;
                display_fan = calculateFanSpeed(data.co2_ppm, co2setpoint);


                snprintf(buf, sizeof(buf), "Auto greenhouse");
                display->text(buf, 0, 0);

                snprintf(buf, sizeof(buf), "CO2:%4.0fppm", data.co2_ppm);
                display->text(buf, 0, 10);

                snprintf(buf, sizeof(buf), "RH: %4.1f%%", data.rh);
                display->text(buf, 0, 19);

                snprintf(buf, sizeof(buf), "T:  %4.1fC", data.temp);
                display->text(buf, 0, 28);

                snprintf(buf, sizeof(buf), "P:  %4.1fPa", data.pressure);
                display->text(buf, 0, 37);

                snprintf(buf, sizeof(buf), "Fan:%3u%%", display_fan);
                display->text(buf, 0, 46);

                snprintf(buf, sizeof(buf), "SET");
                display->text(buf, 95, 20);

                snprintf(buf, sizeof(buf), "%sval", main_selected == 0 ? "*" : " ");
                display->text(buf, 90, 36);

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

            else if (current_screen == ID_SCREEN) {
                snprintf(buf, sizeof(buf), "Wifi set up");
                display->text(buf, 0, 0);

                snprintf(buf, sizeof(buf), "%sset name", id_selected == 0 ? "*" : " ");
                display->text(buf, 20, 18);

                snprintf(buf, sizeof(buf), "%sSet password", id_selected == 1 ? "*" : " ");
                display->text(buf, 20, 30);

                snprintf(buf, sizeof(buf), "%sSend", id_selected == 2 ? "*" : " ");
                display->text(buf, 20, 42);

                snprintf(buf, sizeof(buf), "N:%s", ssid_len > 0 ? "ok" : "--");
                display->text(buf, 0, 55);

                snprintf(buf, sizeof(buf), "P:%s", pass_len > 0 ? "ok" : "--");
                display->text(buf, 40, 55);
            }

            else if (current_screen == SET_NAME || current_screen == SET_PASS) {
                bool is_name = (current_screen == SET_NAME);

                snprintf(buf, sizeof(buf), is_name ? "Set WiFi name:" : "Set WiFi pass:");
                display->text(buf, 0, 0);

                int prev = (char_index - 1 + CHAR_COUNT) % CHAR_COUNT;
                int next = (char_index + 1) % CHAR_COUNT;
                snprintf(buf, sizeof(buf), "< %c  [%c]  %c >",
                         CHAR_LIST[prev], CHAR_LIST[char_index], CHAR_LIST[next]);
                display->text(buf, 0, 18);

                snprintf(buf, sizeof(buf), "%s", is_name ? ssid_input : pass_input);
                display->text(buf, 0, 35);

                snprintf(buf, sizeof(buf), "ENC=add SW1=del");
                display->text(buf, 0, 48);

                snprintf(buf, sizeof(buf), "SW2=save SW0=disc");
                display->text(buf, 0, 57);
            }

            display->show();
            needs_redraw = false;
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}