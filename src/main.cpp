#include <iostream>
#include <sstream>
#include <cstring>

#include "FreeRTOS.h"
#include "task.h"
#include "hardware/gpio.h"
#include "PicoOsUart.h"
#include "pico/stdio.h"
#include "SensorTask.h"
#include "network/remote.h"
#include "structs.h"
#include "hardware/timer.h"
#include "eeprom/eeprom.h"

extern "C" {
    uint32_t read_runtime_ctr(void) {
        return timer_hw->timerawl;
    }
}

int main() {
    stdio_init_all();

    EEPROMManager eeprom(i2c0, 16, 17);

    if (!eeprom.init()) {
        printf("Failed to init eeprom.\n");
        EEPROM_ENABLED = false;
    } else {
        EEPROM_ENABLED = true;
        printf("EEPROM initialized.\n");
    }

    QueueHandle_t receive_queue = xQueueCreate(10, sizeof(message));
    QueueHandle_t co2_que = xQueueCreate(10, sizeof(message));

    static SensorTask sensor_task;
    static RemoteController remote_controller(eeprom, receive_queue, co2_que);

    sensor_task.start();

    // Optional bootstrap credentials for testing.
    // Comment this block out if you want EEPROM-only startup.
    // message msg{};
    // msg.type = NETWORK_CONFIG;
    // strncpy(msg.network_config.ssid, "DNA-WIFI-1878", sizeof(msg.network_config.ssid) - 1);
    // strncpy(msg.network_config.password, "X4xfLfRY1xwQGC", sizeof(msg.network_config.password) - 1);
    // msg.network_config.ssid[sizeof(msg.network_config.ssid) - 1] = '\0';
    // msg.network_config.password[sizeof(msg.network_config.password) - 1] = '\0';
    // xQueueSendToBack(receive_queue, &msg, pdMS_TO_TICKS(10));

    vTaskStartScheduler();

    while (true) {}
}