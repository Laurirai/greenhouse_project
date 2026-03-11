#ifndef UITASK_H
#define UITASK_H

#include <memory>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "SensorTask.h"
#include "ssd1306os.h"
#include "PicoI2C.h"
#include "eeprom/eeprom.h"
#include "structs.h"

class UITask {
public:
    explicit UITask(QueueHandle_t uiQueue, QueueHandle_t inputQueue, EEPROMManager &eeprom_, std::shared_ptr<PicoI2C> i2c);
    void start();
private:
    static void taskFunction(void* param);
    void run();

    QueueHandle_t uiQueue;
    QueueHandle_t inputQueue;

    EEPROMManager &eeprom;

    std::shared_ptr<PicoI2C> i2c;
    std::unique_ptr<ssd1306os> display;
};

#endif