//
// Created by janie on 04/03/2026.
//

#include "remote.h"

RemoteController::RemoteController(const char *ssid_, const char *password_, QueueHandle_t rcq) {
    ssid = ssid_;
    password=password_;
    receive_que=rcq;

    xTaskCreate(taskFunction,
    "remote",
    1024,
    this,
    2,
    NULL);
}

void RemoteController::taskFunction(void *param) {
    static_cast<RemoteController*>(param) -> run();
    vTaskDelete(nullptr);
}

void RemoteController::readChannel(IPStack &ipstack) {
    // only continue if we're connected.
    if (!cloudConnected || !ipstack.WiFi_connected()) {
        printf("Can't read channel when WiFi not connected.");
        cloudConnected = false; // make sure it's false afterthis.
        return;
    }

    //https://api.thingspeak.com/update?api_key=YOUR WRITE API KEY&field6=0.002039
    uint32_t num = 1;
    char req[512];
    snprintf(req,sizeof(req),
        "GET /update?api_key=%s&field1=%d&field2= HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n"
        "\r\n",
        write_api,
        num,
         host);

    ipstack.write((unsigned char *)(req),strlen(req));
    vTaskDelay(pdMS_TO_TICKS(2000));
    printf("Request: %s\n", req);
    auto rv = ipstack.read((unsigned char*) buffer, BUFSIZE, 100);
    printf("HTTP response: %s\n", buffer);

}

bool RemoteController::sendData(IPStack &ip_stack, sensorData data) {
    if (!cloudConnected || !ip_stack.WiFi_connected()) {
        printf("Can't read channel when WiFi not connected.");
        cloudConnected = false; // make sure it's false afterthis.
        return false;
    }

    // where to store msg
    char req[512];
    snprintf(req,sizeof(req),
    "GET /update?api_key=%s&field1=%d&field2=%f&field3=%f&field4=%d&field5=%dHTTP/1.1\r\n"
    "Host: %s\r\n"
    "Connection: close\r\n"
    "\r\n",write_api,data.co2,data.humidity,data.temperature,data.fan_speed,data.co2sp,host);

    int rv =ip_stack.write((unsigned char*)(req), strlen(req));
    if (rv < 0) {
        // failed to send.
        printf("Failed to send data to cloud.\n");
        return false;
    }
    // 1 sec delay and get response.
    vTaskDelay(pdMS_TO_TICKS(1000));

    // not sure if we have to read to empty ipstack buffer or smthing?
    // ip_stack.read((unsigned char*)buffer, BUFSIZE, 100);

    // printf("HTTP response: %s\n", buffer);
    return true;
}


void RemoteController::run() {
    IPStack ip_stack;
    connectCloud(ip_stack);
    printf("Hiya! %d\n", cloudConnected);
    TickType_t last_send = xTaskGetTickCount();
    bool first_send = true;
    while (true) {
        message msg{};
        if (xQueueReceive(receive_que, &msg, pdMS_TO_TICKS(10))) {
            printf("got a message with type: %d\n", msg.type);
            switch (msg.type) {
                case SENSOR_DATA:
                    printf("Received sensor data.\n");
                    if (ip_stack.WiFi_connected()) {
                        TickType_t now = xTaskGetTickCount();
                        if ((now-last_send) >= period || first_send) {
                            last_send = now;
                            printf("Sending data...");
                            if (!sendData(ip_stack, msg.data)) {
                                // failed to send data to cloud for god knows why, log it.
                                printf("Failed to send data to thingspeak.\n");
                            }
                            first_send = false;
                        }
                    }
                    break;
                case NETWORK_CONFIG:
                    printf("Received network config info.\n");
                    ssid = msg.network_config.ssid;
                    password =msg.network_config.password;
                    disconnect(ip_stack);
                    connectCloud(ip_stack);
                    break;
                case CO2SET:
                    printf("Received CO2val\n");
                    break;
                default:
                    printf("no way we should be here but oh well. type: %d\n", msg.type);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

//disconnect from wifi & close tcp connection
void RemoteController::disconnect(IPStack &ip_stack) {
    ip_stack.disconnect_WiFi();
    cloudConnected = false;
}
bool RemoteController::connectCloud(IPStack &ip_stack) {
    if (!ip_stack.connect_WiFi(ssid, password, NETWORK_ATTEMPT_COUNT)) {
        cloudConnected = false;
        return false;
    }
    printf("Succesfully connected to WiFi \nAttempting TCP connection.\n");


    // create tcp connection.
    int rc = ip_stack.connect(host,port);
    vTaskDelay(pdMS_TO_TICKS(5000)); // 5 sec delay to make sure tcp is connected
    if (rc != 0 || !ip_stack.TCP_connected()) {
        printf("Failed to connect due to %d\n", rc);
        ip_stack.disconnect_WiFi(); // full reset if something died.
        return false;
    }
    cloudConnected = true;

    printf("Fully connected.\n");
    return true;
}

