#ifndef _PULSE_COUNTER_DATA_H_
#define _PULSE_COUNTER_DATA_H_

#include <freertos/FreeRTOS.h>

struct PulseCounterData
{
    uint64_t last_pulse_width_us;
    uint64_t pulse_count;

    uint64_t last_detected_edge_us;
    uint64_t last_valid_edge_us;

    public:
        void initialize() {
            last_pulse_width_us = 0;
            pulse_count = 0;
            last_detected_edge_us = 0;
            last_valid_edge_us = 0;
        }
};

#endif