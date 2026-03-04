//
// Created by Skorppari on 04/03/2026.
//
#pragma once

#ifndef GREENHOUSE_PROJECT_SENSORTASK_H
#define GREENHOUSE_PROJECT_SENSORTASK_H
class SensorTask
{
public:
    void start();

private:
    static void taskFunction(void* param);
    void run();
};
#endif //GREENHOUSE_PROJECT_SENSORTASK_H