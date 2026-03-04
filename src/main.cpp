#include <iostream>
#include <sstream>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "hardware/gpio.h"
#include "PicoOsUart.h"
#include "pico/stdio.h"
#include "SensorTask.h"


#include "hardware/timer.h"
extern "C" {
uint32_t read_runtime_ctr(void) {
    return timer_hw->timerawl;
}
}

int main() {
    stdio_init_all();

    static SensorTask sensor_task;
    //static UiTask uiTask;
    sensorTask.start();

    vTaskStartScheduler();

    while (1); // should nevah cum hia
}
