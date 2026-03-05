//
// Created by janie on 04/03/2026.
//

#ifndef STRUCTS_H
#define STRUCTS_H
#include <cstdint>

#define DATA_INTERVAL 60            // send data to thingspeak once every minute
#define NETWORK_ATTEMPT_COUNT 3     // how many times to attempt connection
struct sensorData {
    uint16_t co2; // ppm
    double humidity; // duh
    double temperature; // duh
    uint16_t fan_speed; // 0-100%
    uint16_t co2sp; // set point in ppm
};

struct networkConfig {
    const char *ssid;
    const char* password;
};


enum messageType {
    SENSOR_DATA,
    NETWORK_CONFIG,
    CO2SET
};

struct message {
    messageType type;

    sensorData data;
    networkConfig network_config;
    uint co2set;
};

#endif //STRUCTS_H
