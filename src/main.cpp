#include <iostream>
#include <sstream>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "hardware/gpio.h"
#include "PicoOsUart.h"
#include "pico/stdio.h"
#include "SensorTask.h"
#include "network/remote.h"
#include "structs.h"


#include "hardware/timer.h"
#include "network/remote.h"

extern "C" {
uint32_t read_runtime_ctr(void) {
    return timer_hw->timerawl;
}
}

int main() {
    stdio_init_all();
    const char* SSID = "DNA-WIFI-1878";
    const char* password = "X4xfLfRY1xwQGC";

    // send sensor readings into this que
    QueueHandle_t receive_queue = xQueueCreate(10, sizeof(message));

    static SensorTask sensor_task;
    // create RemoteController and give it all required vars.
    static RemoteController remote_controller(SSID, password, receive_queue);
    //static UiTask uiTask;
    sensor_task.start();


    vTaskStartScheduler();
    while (1); // should nevah cum hia
}
