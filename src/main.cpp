#include <iostream>
#include <sstream>
#include <cstring>

#include "FreeRTOS.h"
#include "task.h"
#include "hardware/gpio.h"
#include "PicoOsUart.h"
#include "queue.h"
#include "pico/stdio.h"
#include "SensorTask.h"
#include "network/remote.h"
#include "structs.h"
#include "hardware/timer.h"
#include "eeprom/eeprom.h"
#include "SensorTask.h"
#include "UITask.h"
#include "inputs/InputHandler.h"
#include "control/ControlTask.h"
#include "ModbusClient.h"
#include "PicoOsUart.h"
#include "PicoI2C.h"
#include "semphr.h"

extern "C" {
    uint32_t read_runtime_ctr(void) {
        return timer_hw->timerawl;
    }
}

QueueHandle_t uiQueue;

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

    static std::shared_ptr<PicoOsUart>   uart       = std::make_shared<PicoOsUart>(1, 4, 5, 9600, 2);
    static std::shared_ptr<ModbusClient> modbus      = std::make_shared<ModbusClient>(uart);
    static std::shared_ptr<PicoI2C>      shared_i2c  = std::make_shared<PicoI2C>(1, 400000);
    SemaphoreHandle_t modbusMutex = xSemaphoreCreateMutex();

    QueueHandle_t receive_queue = xQueueCreate(10, sizeof(message));
    QueueHandle_t co2_que       = xQueueCreate(10, sizeof(message));
    QueueHandle_t controlQueue  = xQueueCreate(5, sizeof(SensorData));
    uiQueue                     = xQueueCreate(5, sizeof(SensorData));

    static RemoteController remote_controller(eeprom, receive_queue, co2_que);


    static SensorTask   sensorTask(uiQueue, controlQueue, modbus, modbusMutex, shared_i2c, receive_queue);
    static InputHandler inputHandler;
    static UITask       uiTask(uiQueue, inputHandler.getQueue(), eeprom, shared_i2c, receive_queue);
    static ControlTask  controlTask(controlQueue, eeprom, modbus, modbusMutex);

    sensorTask.start();
    uiTask.start();
    controlTask.start();

    vTaskStartScheduler();
    while (true);
}