//
// Created by Herra_HUU on 07/03/2026.
//

#ifndef INPUTHANDLER_H
#define INPUTHANDLER_H

#include "FreeRTOS.h"
#include "queue.h"
#include "hardware/gpio.h"

enum InputEvent {
    BTN_RIGHT,    // SW0 - cursor right
    BTN_BACK,     // SW1 - previous screen
    BTN_LEFT,     // SW2 - cursor left
    ENC_UP,       // scroll up through chars / menu up
    ENC_DOWN,     // scroll down through chars / menu down
    ENC_PRESS     // select / confirm
};

class InputHandler {
public:
    InputHandler();
    QueueHandle_t getQueue() { return eventQueue; }

private:
    static void gpioCallback(uint gpio, uint32_t events);
    void handleGpio(uint gpio, uint32_t events);

    static InputHandler* instance;
    QueueHandle_t eventQueue;

    // debounce timestamps
    uint32_t last_sw0 = 0;
    uint32_t last_sw1 = 0;
    uint32_t last_sw2 = 0;
    uint32_t last_enc_sw = 0;
    uint32_t last_turn = 0;
};

#endif