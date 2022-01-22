#include "PulseMeter.h"
#include <esp_timer.h>
#include <iostream>
#include <driver/gpio.h>
#include <esp_log.h>

using namespace std;

// PulseMeter::PulseMeter(const int gpio,const uint32_t filter_us = 0) : filter_us(filter_us){

// };

void PulseMeter::setup() {
    this->last_detected_edge_us = 0;
    this->last_valid_edge_us = 0;
    this->timeout_us = 300000000; // usec
    this->filter_us = 100000; //13; // usec

    this->isr_pin = new ISRInternalGPIOPin(
        GPIO_NUM_32,
        GPIO_DRIVE_CAP_DEFAULT,
        false,
        gpio::FLAG_INPUT
    );
    this->isr_pin->setup();
    this->isr_pin->attach_interrupt(PulseMeter::gpio_intr, this, InterruptType::INTERRUPT_ANY_EDGE);
}

void PulseMeter::update() {
    const int64_t now = esp_timer_get_time();
    const int64_t since_last_edge_us = now - last_valid_edge_us; // microseconds

    const uint32_t time_since_valid_edge_us = now - this->last_valid_edge_us;
    if ((this->last_valid_edge_us != 0) && (time_since_valid_edge_us > this->timeout_us)) {
        printf("No pulse detected for %us, assuming 0 pulses/min\n", time_since_valid_edge_us / 1000000);
        this->last_valid_edge_us = 0;
        this->pulse_width_us = 0;
    }

    const uint32_t pulse_width_ms = this->pulse_width_us / 1000;
    if (this->pulse_width_dedupe.next(pulse_width_ms)) {
        if (pulse_width_ms == 0) {
            printf("No pulse width\n");
        } else {
            printf("Count %lf\n", ((60.0f * 1000.0f) / pulse_width_ms));
        }
    }

    const uint32_t total_pulses = this->total_pulses;
    if (this->total_dedupe.next(total_pulses)) {
        printf("Total pulses: %d\n", total_pulses);
    }
}

void IRAM_ATTR PulseMeter::gpio_intr(void *sensorVoid) {
    // This is an interrupt handler - we can't call any virtual method from this method

    // Get the current time before we do anything else so the measurements are consistent

    PulseMeter* sensor = reinterpret_cast<PulseMeter*>(sensorVoid);
    const uint32_t now = esp_timer_get_time();

    
    // We only look at rising edges
    if (!sensor->isr_pin->digital_read_isr()) {
        ets_printf("Not rising edge\n");
        return;
    }

    // Check to see if we should filter this edge out
    if ((now - sensor->last_detected_edge_us) >= sensor->filter_us) {
        ets_printf("Filtered: %d, Last valid us: %d", (now - sensor->last_detected_edge_us), sensor->last_valid_edge_us);
        // Don't measure the first valid pulse (we need at least two pulses to measure the width)
        if (sensor->last_valid_edge_us != 0) {
            sensor->pulse_width_us = (now - sensor->last_valid_edge_us);
        }

        sensor->total_pulses++;
        sensor->last_valid_edge_us = now;
    }

    sensor->last_detected_edge_us = now;
}