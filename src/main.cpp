#include <stdio.h>
#include <driver/gpio.h>
#include <driver/adc.h>
#include <iostream>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_err.h>
#include <nvs_flash.h>
#include "esp_sleep.h"
#include "esp_attr.h"
#include "rom/rtc.h"
#include "rom/ets_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/uart_reg.h"
#include "soc/timer_group_reg.h"
#include "PulseMeter.h"
#include "PulseCounter.h"
#include "rtc_safe_utils.h"
#include <esp_system.h>

extern "C" void app_main(void);

#define PULSE_GPIO_NUM GPIO_NUM_32

const RTC_DATA_ATTR uint64_t WAKE_UP_TIME = 0 * 1000 * 1000; // 30s
const uint64_t INTERNAL_FILTER_US = 100 * 1000; // 100ms
const uint64_t PULSE_TIMEOUT_US = 5 * 60 * 1000 * 1000; // 5min

PulseMeter pulseMeter;

// Pulse counter value, stored in RTC_SLOW_MEM
static uint64_t RTC_DATA_ATTR last_wake_time_us;
static PulseCounterConfig RTC_DATA_ATTR pulse_config {
    .gpio_num = PULSE_GPIO_NUM,
    .drive_cap = GPIO_DRIVE_CAP_0,
    .filter_us = INTERNAL_FILTER_US,
    .pulse_timeout_us = PULSE_TIMEOUT_US
};
static PulseCounterData RTC_DATA_ATTR data;

// Function which runs after exit from deep sleep
static void RTC_IRAM_ATTR wake_stub_pulse_counter();

void app_main(void) {

    auto wakeup_reason = rtc_get_reset_reason(0);

    if (wakeup_reason == DEEPSLEEP_RESET) {
        gpio_reset_pin(GPIO_NUM_33);
        gpio_set_direction(GPIO_NUM_33, GPIO_MODE_OUTPUT);
        gpio_set_level(GPIO_NUM_33, 1);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        gpio_set_level(GPIO_NUM_33, 0);
        printf("Wake up from deep sleep\n");
        printf("Pulse count=%lld\n", data.pulse_count);
        if (data.last_pulse_width_us > 0) {
            const uint32_t pulse_width_ms = data.last_pulse_width_us / 1000;
            auto consume = (60.0f * 1000.0f) / pulse_width_ms;
            printf("Consume: %lf\n", consume);
        }
    } else {
        // first boot
        printf("First boot...setting up initialization phase\n");
        last_wake_time_us = 0;
        data.initialize();
    }

    // Setup pulse counter pins
    PulseCounter::setup(&pulse_config);

    // TODO: send data using ESP-NOW

    printf("Going to deep...\n");
    
    // reatain gpio pad and Enter deep sleep
    gpio_hold_en(pulse_config.gpio_num);
    gpio_deep_sleep_hold_en();
    esp_set_deep_sleep_wake_stub(&wake_stub_pulse_counter);
    esp_sleep_enable_ext1_wakeup(1ULL << static_cast<uint32_t>(pulse_config.gpio_num), ESP_EXT1_WAKEUP_ANY_HIGH);
    
    // set last wake time before go to sleep. Assigned here automatically exclude the time spent while is wake.
    last_wake_time_us = rtc_time_get_us();
    esp_deep_sleep_start();
};

static void RTC_IRAM_ATTR wake_stub_pulse_counter()
{
    const uint64_t now = rtc_time_get_us();

    // We wait for signal go to low, looking at rising edges.
    // So while is true wait
    while (!PulseCounter::is_low(&pulse_config));

    PulseCounter::update_pulses_if_needed(&data, &pulse_config, now);
    
    if (now - last_wake_time_us >= WAKE_UP_TIME) {
        esp_default_wake_deep_sleep();
        esp_wake_deep_sleep();
        return;
    }

    // flush uart buffer
    while (REG_GET_FIELD(UART_STATUS_REG(0), UART_ST_UTX_OUT));
    // set wake stub
    REG_WRITE(RTC_ENTRY_ADDR_REG, (uint32_t)&wake_stub_pulse_counter);
    // Go to sleep.
    CLEAR_PERI_REG_MASK(RTC_CNTL_STATE0_REG, RTC_CNTL_SLEEP_EN);
    SET_PERI_REG_MASK(RTC_CNTL_STATE0_REG, RTC_CNTL_SLEEP_EN);
    // A few CPU cycles may be necessary for the sleep to start...
    ets_delay_us(100);
}