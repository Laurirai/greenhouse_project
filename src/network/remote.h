//
// Created by janie on 04/03/2026.
//
#pragma once
#ifndef REMOTE_H
#define REMOTE_H

#include "FreeRTOS.h"
#include "task.h"
#include "ipstack/IPStack.h"
#include "queue.h"
#include "structs.h"


class RemoteController {
public:
    RemoteController(const char* ssid_, const char* password_, QueueHandle_t rcq);

private:
    static void taskFunction(void* param);
    void run();

    bool sendData(IPStack &ip_stack,sensorData data);

    bool connectCloud(IPStack & ip_stack);

    void disconnect(IPStack &ip_stack);
    bool cloudConnected = false;

    void readChannel(IPStack &ip_stack);
    static const int BUFSIZE = 2048;
    char buffer[BUFSIZE] = {};
    const char*ssid = nullptr;
    const char*password = nullptr;

    const char* name = "TEST";
    const char *host = "api.thingspeak.com";
    const int port = 80;
    const int channel = 3268971;
    const char *talkback_key = "9BWJRPNQRSGVKPDB";
    const char *write_api = "6J1VQ3JWJGPNWKZH";
    const char *read_api = "H9OQ8XDTI7OU4IUW";

    // que to receive data from.
    QueueHandle_t receive_que{};

    const TickType_t period = pdMS_TO_TICKS(DATA_INTERVAL*1000); // in ms

};
#endif

