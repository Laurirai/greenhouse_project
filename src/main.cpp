#include <cstdio>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "pico/stdio.h"
#include "hardware/timer.h"
#include "SensorTask.h"

extern "C" {
    uint32_t read_runtime_ctr(void) {
        return timer_hw->timerawl;
    }
}

// Global queues
QueueHandle_t uiQueue;

int main() {
    stdio_init_all();

    // Create queues (holds up to 5 SensorData items each)
    uiQueue = xQueueCreate(5, sizeof(SensorData));

    // Create tasks
    static SensorTask sensorTask(uiQueue);
    sensorTask.start();

    // UITask comes next - placeholder for now
    // static UITask uiTask(uiQueue);
    // uiTask.start();

    vTaskStartScheduler();
    while (true);
}