#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "pico/stdio.h"
#include "hardware/timer.h"
#include "SensorTask.h"
#include "UITask.h"
#include "inputs/InputHandler.h"

extern "C" {
    uint32_t read_runtime_ctr(void) {
        return timer_hw->timerawl;
    }
}

QueueHandle_t uiQueue;

int main() {
    stdio_init_all();

    uiQueue = xQueueCreate(5, sizeof(SensorData));

    static SensorTask sensorTask(uiQueue);
    static InputHandler inputHandler;
    static UITask uiTask(uiQueue, inputHandler.getQueue());

    sensorTask.start();
    uiTask.start();

    vTaskStartScheduler();
    while (true);
}