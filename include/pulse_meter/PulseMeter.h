#ifndef _PULSE_METER_H_
#define _PULSE_METER_H_

#include <freertos/FreeRTOS.h>
#include "Deduplicator.h"
#include "ISRInternalGPIOPin.h"

class PulseMeter {

    public:
        PulseMeter() = default;
        void setup();
        void update();

    private:
        ISRInternalGPIOPin* isr_pin;
        uint32_t filter_us = 0;
        uint32_t last_detected_edge_us = 0;
        uint32_t last_valid_edge_us = 0;
        uint32_t pulse_width_us = 0;
        uint32_t total_pulses = 0;
        uint32_t timeout_us = 0;

        Deduplicator<uint32_t> pulse_width_dedupe;
        Deduplicator<uint32_t> total_dedupe;

        static void gpio_intr(void *sensorVoid);
};

#endif