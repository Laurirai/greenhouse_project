#ifndef REMOTE_H
#define REMOTE_H

#include "ipstack.h"
#include "eeprom/eeprom.h"
#include "structs.h"
#include "FreeRTOS.h"
#include "queue.h"

class RemoteController {
public:
    RemoteController(EEPROMManager &eeprom_, QueueHandle_t rcq, QueueHandle_t co2q);

    static void taskFunction(void *param);
    void run();

private:
    EEPROMManager &eeprom;

    QueueHandle_t receive_que{};
    QueueHandle_t to_co2_queue{};

    bool cloudConnected = false;

    static constexpr TickType_t send_period = pdMS_TO_TICKS(DATA_INTERVAL * 1000);
    static constexpr TickType_t talkback_period = pdMS_TO_TICKS(15000);
    static constexpr TickType_t reconnect_period = pdMS_TO_TICKS(5000);

    char ssid[32]{};
    char password[64]{};
    char buffer[1024]{};

    const char *host = "api.thingspeak.com";
    const int port = 80;
    const char *write_api = "6J1VQ3JWJGPNWKZH";
    const char *talkback_id = "56511";
    const char *talkback_api = "9BWJRPNQRSGVKPDB";


    bool connectCloud(IPStack &ip_stack);
    void disconnect(IPStack &ip_stack);

    bool openTCP(IPStack &ip_stack);
    void closeTCP(IPStack &ip_stack);

    bool readHttpResponse(IPStack &ip_stack);
    bool sendData(IPStack &ip_stack, const sensorData &data);
    bool parseTalkBackCommand(const char *body, uint32_t &co2_value);
    void forwardCO2Set(uint32_t co2_value);
    bool readTalkBackCO2(IPStack &ip_stack);
};

#endif // REMOTE_H