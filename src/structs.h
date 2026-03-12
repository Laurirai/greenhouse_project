//
// Created by janie on 04/03/2026.
//

#ifndef STRUCTS_H
#define STRUCTS_H

#include <cstdint>

#define DATA_INTERVAL 60            // seconds
#define TALKBACK_INTERVAL 5         // seconds
#define NETWORK_ATTEMPT_COUNT 3
#define MIN_CO2_SET 200 // min that's accepted from remote
#define DEFAULT_CO2_SET 800
#define MAX_CO2_SET 5000 // max that's accepted from remote

inline uint32_t co2setpoint;
inline bool EEPROM_ENABLED = false;

struct sensorData {
    uint16_t co2;          // ppm
    double humidity;
    double temperature;
    uint16_t fan_speed;    // 0-100%
    uint16_t co2sp;        // set point in ppm
};

struct networkConfig {
    char ssid[32] = {0};
    char password[64] = {0};
};

enum messageType {
    SENSOR_DATA,
    NETWORK_CONFIG,
};

struct message {
    messageType type = SENSOR_DATA;
    sensorData data{};
    networkConfig network_config{};
    uint32_t co2set = 0;
};

#endif // STRUCTS_H