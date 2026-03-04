//
// Created by HerraHuu on 04/03/2026.
//
#include "FreeRTOS.h"
#include "task.h"
#include "SensorTask.h"

void SensorTask::start()
{
    xTaskCreate(taskFunction, // creating the task handler Mrs. Electrode
                "Sensor",
                1024,
                this,
                2,
                NULL);
}

void SensorTask::taskFunction(void* param)
{
    static_cast<SensorTask*>(param)->run();
}

void SensorTask::run()
{
    while(true)
    {
        // Do sensor stuffff
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}