//
// Created by Herra HUU on 07/03/2026.
//

#include "InputHandler.h"
#include "hardware/gpio.h"
#include "pico/time.h"

#define SW0_PIN   9
#define SW1_PIN   8
#define SW2_PIN   7
#define ROT_A_PIN 10
#define ROT_B_PIN 11
#define ROT_SW_PIN 12
#define DEBOUNCE_MS 120
#define BUTTON_DB 300

InputHandler* InputHandler::instance = nullptr;

InputHandler::InputHandler() {
    instance = this;
    eventQueue = xQueueCreate(10, sizeof(InputEvent));

    // init all pins as input with pull-up
    uint pins[] = {SW0_PIN, SW1_PIN, SW2_PIN, ROT_A_PIN, ROT_B_PIN, ROT_SW_PIN};
    for (auto pin : pins) {
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_up(pin);
    }

    // attach interrupts - falling edge (button press pulls low)
    gpio_set_irq_enabled_with_callback(SW0_PIN,   GPIO_IRQ_EDGE_FALL, true, &gpioCallback);
    gpio_set_irq_enabled(SW1_PIN,   GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(SW2_PIN,   GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(ROT_A_PIN, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(ROT_SW_PIN,GPIO_IRQ_EDGE_FALL, true);
}

void InputHandler::gpioCallback(uint gpio, uint32_t events) {
    if (instance) instance->handleGpio(gpio, events);
}

void InputHandler::handleGpio(uint gpio, uint32_t events) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    InputEvent ev;
    uint32_t now = to_ms_since_boot(get_absolute_time());

    if (gpio == SW0_PIN) {
        if (now - last_sw0 < BUTTON_DB) return;
        last_sw0 = now;
        ev = BTN_RIGHT;
        xQueueSendToBackFromISR(eventQueue, &ev, &xHigherPriorityTaskWoken);
    }
    else if (gpio == SW1_PIN) {
        if (now - last_sw1 < BUTTON_DB) return;
        last_sw1 = now;
        ev = BTN_BACK;
        xQueueSendToBackFromISR(eventQueue, &ev, &xHigherPriorityTaskWoken);
    }
    else if (gpio == SW2_PIN) {
        if (now - last_sw2 < BUTTON_DB) return;
        last_sw2 = now;
        ev = BTN_LEFT;
        xQueueSendToBackFromISR(eventQueue, &ev, &xHigherPriorityTaskWoken);
    }
    else if (gpio == ROT_A_PIN) {
        if (now - last_turn < 10) return;
        last_turn = now;
        ev = (gpio_get(ROT_B_PIN) == 0) ? ENC_DOWN : ENC_UP;
        xQueueSendToBackFromISR(eventQueue, &ev, &xHigherPriorityTaskWoken);
    }
    else if (gpio == ROT_SW_PIN) {
        if (now - last_enc_sw < 300) return;
        last_enc_sw = now;
        ev = ENC_PRESS;
        xQueueSendToBackFromISR(eventQueue, &ev, &xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}