//
// Created by janie on 04/03/2026.
//

#include "remote.h"
#include <cstdio>
#include <cstring>
#include <cctype>

RemoteController::RemoteController(EEPROMManager &eeprom_, QueueHandle_t rcq, QueueHandle_t co2q)
    : eeprom(eeprom_) {
    receive_que = rcq;
    to_co2_queue = co2q;

    if (EEPROM_ENABLED) {
        networkConfig cfg{};

        if (eeprom.loadNetworkConfig(cfg)) {
            strncpy(ssid, cfg.ssid, sizeof(ssid) - 1);
            strncpy(password, cfg.password, sizeof(password) - 1);
            ssid[sizeof(ssid) - 1] = '\0';
            password[sizeof(password) - 1] = '\0';

            if (strlen(ssid) >= 2 && strlen(password) >= 8) {
                printf("Read SSID & password from eeprom with vals: %s | %s\n", ssid, password);
            } else {
                printf("Trash data in EEPROM for SSID/pass, ignoring.\n");
                memset(ssid, 0, sizeof(ssid));
                memset(password, 0, sizeof(password));
            }
        } else {
            printf("Failed to read creds from eeprom\n");
        }

    }

    xTaskCreate(taskFunction,
                "remote",
                4096,
                this,
                tskIDLE_PRIORITY + 2,
                NULL);
}

void RemoteController::taskFunction(void *param) {
    static_cast<RemoteController*>(param)->run();
    vTaskDelete(nullptr);
}

bool RemoteController::connectCloud(IPStack &ip_stack) {
    if (strlen(ssid) < 2 || strlen(password) < 8) {
        printf("No valid credentials loaded.\n");
        cloudConnected = false;
        return false;
    }

    if (ip_stack.WiFi_connected()) {
        cloudConnected = true;
        return true;
    }

    if (!ip_stack.connect_WiFi(ssid, password, NETWORK_ATTEMPT_COUNT)) {
        printf("Failed to connect to WiFi.\n");
        cloudConnected = false;
        return false;
    }

    printf("Succesfully connected to WiFi.\n");
    cloudConnected = true;
    return true;
}

void RemoteController::disconnect(IPStack &ip_stack) {
    ip_stack.disconnect();
    ip_stack.disconnect_WiFi();
    cloudConnected = false;
}

bool RemoteController::openTCP(IPStack &ip_stack) {
    if (!cloudConnected || !ip_stack.WiFi_connected()) {
        printf("Can't open TCP, WiFi not connected.\n");
        cloudConnected = false;
        return false;
    }

    int rc = ip_stack.connect(host, port);
    if (rc != 0) {
        printf("Failed TCP connect due to %d\n", rc);
        closeTCP(ip_stack);
        return false;
    }

    for (int i = 0; i < 20; ++i) {
        if (ip_stack.TCP_connected()) {
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    printf("TCP connect timed out after successful connect() call.\n");
    closeTCP(ip_stack);
    return false;
}

void RemoteController::closeTCP(IPStack &ip_stack) {
    ip_stack.disconnect();
}

bool RemoteController::sendData(IPStack &ip_stack, const sensorData &data) {
    if (!cloudConnected || !ip_stack.WiFi_connected()) {
        printf("Can't send data when WiFi not connected.\n");
        cloudConnected = false;
        return false;
    }

    if (!openTCP(ip_stack)) {
        printf("Failed to open TCP for sendData.\n");
        return false;
    }

    char req[512];
    memset(req, 0, sizeof(req));
    memset(buffer, 0, sizeof(buffer));

    snprintf(req, sizeof(req),
             "GET /update?api_key=%s&field1=%u&field2=%.2f&field3=%.2f&field4=%u&field5=%u HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n"
             "\r\n",
             write_api,
             data.co2,
             data.humidity,
             data.temperature,
             data.fan_speed,
             data.co2sp,
             host);

    int rv = ip_stack.write((unsigned char*)req, strlen(req));
    if (rv < 0) {
        printf("Failed to send data to cloud.\n");
        closeTCP(ip_stack);
        return false;
    }

    rv = ip_stack.read((unsigned char*)buffer, sizeof(buffer) - 1, 2000);
    if (rv <= 0) {
        printf("No HTTP response read after sendData.\n");
        closeTCP(ip_stack);
        return false;
    }

    buffer[rv] = '\0';

    if (strstr(buffer, "HTTP/1.1 200 OK") == nullptr) {
        printf("ThingSpeak request failed.\n");
        closeTCP(ip_stack);
        return false;
    }

    closeTCP(ip_stack);
    return true;
}

bool RemoteController::parseTalkBackCommand(const char *body, uint32_t &co2_value) {
    if (body == nullptr) return false;

    while (*body == '\r' || *body == '\n' || *body == ' ' || *body == '\t') {
        body++;
    }

    if (*body == '\0') return false;

    if (sscanf(body, "%u", &co2_value) == 1) return true;
    if (sscanf(body, "CO2SET %u", &co2_value) == 1) return true;
    if (sscanf(body, "co2_set=%u", &co2_value) == 1) return true;
    if (sscanf(body, "co2set=%u", &co2_value) == 1) return true;
    if (sscanf(body, "SETCO2 %u", &co2_value) == 1) return true;
    if (sscanf(body, "co2 %u", &co2_value) == 1) return true;

    return false;
}


bool RemoteController::readTalkBackCO2(IPStack &ip_stack) {
    if (!cloudConnected || !ip_stack.WiFi_connected()) {
        printf("Can't read TalkBack when WiFi not connected.\n");
        cloudConnected = false;
        return false;
    }

    if (!openTCP(ip_stack)) {
        printf("Failed to open TCP for TalkBack.\n");
        return false;
    }

    char req[512];
    memset(req, 0, sizeof(req));
    memset(buffer, 0, sizeof(buffer));

    snprintf(req, sizeof(req),
             "GET /talkbacks/%s/commands/execute?api_key=%s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n"
             "\r\n",
             talkback_id,
             talkback_api,
             host);

    int rv = ip_stack.write((unsigned char*)req, strlen(req));
    if (rv < 0) {
        printf("Failed to send TalkBack request.\n");
        closeTCP(ip_stack);
        return false;
    }

    rv = ip_stack.read((unsigned char*)buffer, sizeof(buffer) - 1, 2000);
    if (rv <= 0) {
        printf("Failed to read TalkBack response.\n");
        closeTCP(ip_stack);
        return false;
    }

    buffer[rv] = '\0';
    // printf("TalkBack response: %s\n", buffer);
    closeTCP(ip_stack);

    if (strstr(buffer, "HTTP/1.1 401 Unauthorized") != nullptr) {
        printf("TalkBack auth failed. Check TalkBack ID / API key.\n");
        return false;
    }

    if (strstr(buffer, "HTTP/1.1 200 OK") == nullptr) {
        printf("TalkBack request failed, ignoring response.\n");
        return false;
    }

    char *body = strstr(buffer, "\r\n\r\n");
    if (body == nullptr) {
        printf("Invalid HTTP response, no body found.\n");
        return false;
    }
    body += 4;

    while (*body == '\r' || *body == '\n' || *body == ' ' || *body == '\t') {
        body++;
    }

    char *actual_body = body;
    char *newline = strstr(body, "\r\n");

    if (newline != nullptr) {
        bool chunk_len = true;
        for (char *p = body; p < newline; ++p) {
            if (!isxdigit((unsigned char)*p)) {
                chunk_len = false;
                break;
            }
        }
        if (chunk_len) {
            actual_body = newline + 2;
        }
    }

    uint32_t co2_value = 0;
    if (*actual_body == '\n' || strcmp(actual_body, "0") == 0) {
        return false;
    }
    if (!parseTalkBackCommand(actual_body, co2_value)) {
        printf("No valid CO2 command found in TalkBack body: %s\n", actual_body);
        return false;
    }

    if (co2_value < MIN_CO2_SET || co2_value > MAX_CO2_SET) {
        printf("Ignoring invalid CO2 setpoint: %u\n", co2_value);
        return false;
    }

    if (EEPROM_ENABLED) {
        if (!eeprom.saveCO2Setpoint(co2_value)) {
            printf("Failed to save CO2 setpoint to EEPROM.\n");
        }
    }

    return true;
}

void RemoteController::run() {
    IPStack ip_stack;

    TickType_t last_send = xTaskGetTickCount();
    TickType_t last_talkback = xTaskGetTickCount();
    TickType_t last_reconnect = xTaskGetTickCount();
    bool first_send = true;

    if (strlen(ssid) >= 2 && strlen(password) >= 8) {
        connectCloud(ip_stack);
    } else {
        printf("No SSID loaded, waiting for network config.\n");
    }

    while (true) {
        message msg{};
        bool got_msg = xQueueReceive(receive_que, &msg, pdMS_TO_TICKS(100));
        TickType_t now = xTaskGetTickCount();

        if (!cloudConnected || !ip_stack.WiFi_connected()) {
            if (strlen(ssid) >= 2 && strlen(password) >= 8 &&
                (now - last_reconnect) >= reconnect_period) {
                last_reconnect = now;
                connectCloud(ip_stack);
            }
        }

        if (got_msg) {

            switch (msg.type) {
                case SENSOR_DATA:
                    if (cloudConnected && ip_stack.WiFi_connected()) {
                        if ((now - last_send) >= send_period || first_send) {

                            printf("Sending sensor data...\n");
                            if (sendData(ip_stack, msg.data)) {
                                last_send = now;
                                first_send = false;
                            }
                            else {
                                printf("Failed to send data to thingspeak.\n");
                            }
                        }
                    }
                    break;

                case NETWORK_CONFIG: {
                    char new_ssid[32]{};
                    char new_password[64]{};

                    strncpy(new_ssid, msg.network_config.ssid, sizeof(new_ssid) - 1);
                    strncpy(new_password, msg.network_config.password, sizeof(new_password) - 1);

                    if (strlen(new_ssid) < 2 || strlen(new_password) < 8) {
                        printf("Ignoring invalid network config.\n");
                        break;
                    }

                    strncpy(ssid, new_ssid, sizeof(ssid) - 1);
                    strncpy(password, new_password, sizeof(password) - 1);
                    ssid[sizeof(ssid) - 1] = '\0';
                    password[sizeof(password) - 1] = '\0';

                    if (EEPROM_ENABLED) {
                        networkConfig cfg{};
                        strncpy(cfg.ssid, ssid, sizeof(cfg.ssid) - 1);
                        strncpy(cfg.password, password, sizeof(cfg.password) - 1);
                        cfg.ssid[sizeof(cfg.ssid) - 1] = '\0';
                        cfg.password[sizeof(cfg.password) - 1] = '\0';

                        if (!eeprom.saveNetworkConfig(cfg)) {
                            printf("Failed to save credentials to EEPROM.\n");
                        } else {
                            printf("Credentials saved to EEPROM.\n");
                        }
                    }

                    disconnect(ip_stack);
                    connectCloud(ip_stack);
                    break;
                }
                default:
                    printf("Unknown message type: %d\n", msg.type);
                    break;
            }
        }

        if (cloudConnected && ip_stack.WiFi_connected()) {
            if ((now - last_talkback) >= talkback_period) {
                last_talkback = now;
                readTalkBackCO2(ip_stack);
            }
        }
    }
}