#ifndef _PULSE_COUNTER_H_
#define _PULSE_COUNTER_H_

#include <freertos/FreeRTOS.h>
#include <driver/gpio.h>
#include <driver/adc.h>
#include <model/PulseCounterData.h>

struct PulseCounterConfig
{
    const gpio_num_t gpio_num;
    const gpio_drive_cap_t drive_cap;
    const uint64_t filter_us;
    const uint64_t pulse_timeout_us;
};

class PulseCounter {
    public:
        static void setup(const PulseCounterConfig* config);
        static void attach_intterupt(const PulseCounterConfig* config, PulseCounterData* data);
        
        static RTC_IRAM_ATTR bool is_low(const gpio_num_t gpio_num); 
        static RTC_IRAM_ATTR void update_pulses_if_needed(PulseCounterData *data, const PulseCounterConfig* config, uint64_t now_us);
};

#endif