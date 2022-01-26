#include "PulseCounter.h"
#include <esp_attr.h>
#include <rom/rtc.h>
#include <driver/rtc_io.h>
#include <soc/rtc_cntl_reg.h>
#include <soc/rtc_io_reg.h>

#define PULSE_CNT_IS_LOW(RTC_GPIO_NUM) \
    ((REG_GET_FIELD(RTC_GPIO_IN_REG, RTC_GPIO_IN_NEXT) \
            & BIT(RTC_GPIO_NUM)) == 0)

static const char RTC_RODATA_ATTR wake_pulse_count[] = "Pulses: %lld\n";

void PulseCounter::setup(const PulseCounterConfig* config)
{
    gpio_config_t conf{};
    conf.pin_bit_mask = 1ULL << static_cast<uint32_t>(config->gpio_num);
    conf.mode = GPIO_MODE_INPUT;
    conf.pull_up_en = GPIO_PULLUP_DISABLE;
    conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&conf);
    gpio_set_drive_capability(config->gpio_num, config->drive_cap);
}

RTC_IRAM_ATTR bool PulseCounter::is_low(const PulseCounterConfig* config) {
    auto gpio_mask = 1ULL << static_cast<uint32_t>(config->gpio_num);
    return ((REG_GET_FIELD(RTC_GPIO_IN_REG, RTC_GPIO_IN_NEXT) & BIT(gpio_mask)) == 0);
}

RTC_IRAM_ATTR void PulseCounter::update_pulses_if_needed(PulseCounterData *data, const PulseCounterConfig* config, uint64_t now_us)
{
    // Check to see if we should filter this edge out
    if ((now_us - data->last_detected_edge_us) >= config->filter_us)
    {
        // ets_printf("Filtered: %d, Last valid us: %d", (now - last_detected_edge_us), last_valid_edge_us);
        //  Don't measure the first valid pulse (we need at least two pulses to measure the width)
        if (data->last_valid_edge_us != 0)
        {
            data->last_pulse_width_us = (now_us - data->last_valid_edge_us);
        }
        data->pulse_count++;
        data->last_valid_edge_us = now_us;
        ets_printf(wake_pulse_count, data->pulse_count);
    }

    data->last_detected_edge_us = now_us;
}