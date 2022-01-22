#include <stdio.h>
#include <driver/gpio.h>
#include <driver/adc.h>
#include <iostream>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event_loop.h>
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

extern "C" void app_main(void);

#define PULSE_CNT_GPIO_NUM GPIO_NUM_32
#define PULSE_CNT_RTC_GPIO_NUM 9

const int INTERNAL_FILTER = 100; // ms

PulseMeter pulseMeter;

// Pulse counter value, stored in RTC_SLOW_MEM
static size_t RTC_DATA_ATTR s_pulse_count;
static size_t RTC_DATA_ATTR s_max_pulse_count;
static uint32_t RTC_DATA_ATTR last_detected_edge_us;
static uint32_t RTC_DATA_ATTR last_valid_edge_us;
static uint32_t RTC_DATA_ATTR filter_us;
static uint32_t RTC_DATA_ATTR total_pulses;

#define PULSE_CNT_IS_LOW() \
    ((REG_GET_FIELD(RTC_GPIO_IN_REG, RTC_GPIO_IN_NEXT) \
            & BIT(PULSE_CNT_RTC_GPIO_NUM)) == 0)

// Function which runs after exit from deep sleep
static void RTC_IRAM_ATTR wake_stub();

void app_main(void) {

    if (rtc_get_reset_reason(0) == DEEPSLEEP_RESET) {
        printf("Wake up from deep sleep\n");
        printf("Pulse count=%d\n", s_pulse_count);
    } else {
        printf("Not a deep sleep wake up\n");
    }

    s_pulse_count = 0;
    s_max_pulse_count = 20;
    last_detected_edge_us = 0;
    last_valid_edge_us = 0;
    total_pulses = 0;
    filter_us = 100000;

    gpio_config_t conf{};
    conf.pin_bit_mask = 1ULL << static_cast<uint32_t>(PULSE_CNT_GPIO_NUM);
    conf.mode = GPIO_MODE_INPUT;
    conf.pull_up_en = GPIO_PULLUP_DISABLE;
    conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&conf);
    gpio_set_drive_capability(PULSE_CNT_GPIO_NUM, GPIO_DRIVE_CAP_DEFAULT);
    gpio_hold_en(PULSE_CNT_GPIO_NUM);

    printf("Going to deep sleep in 1 second\n");
    printf("Will wake up after %d pulses\n", s_max_pulse_count);
    vTaskDelay(1000/portTICK_PERIOD_MS);

    gpio_deep_sleep_hold_en();
    esp_set_deep_sleep_wake_stub(&wake_stub);

    esp_sleep_enable_ext1_wakeup(1ULL << static_cast<uint32_t>(PULSE_CNT_GPIO_NUM), ESP_EXT1_WAKEUP_ANY_HIGH);

    // Enter deep sleep
    esp_deep_sleep_start();
};

static const char RTC_RODATA_ATTR wake_pulse_count[] = "Pulses: %d\n";
static const char RTC_RODATA_ATTR no_rising_edge[] = "Not rising edge\n";
static const char RTC_RODATA_ATTR max_reached[] = "Max reached \n";

static uint64_t RTC_IRAM_ATTR rtc_time_get()
{
    SET_PERI_REG_MASK(RTC_CNTL_TIME_UPDATE_REG, RTC_CNTL_TIME_UPDATE);
    while (GET_PERI_REG_MASK(RTC_CNTL_TIME_UPDATE_REG, RTC_CNTL_TIME_VALID) == 0) {
        ets_delay_us(1); // might take 1 RTC slowclk period, don't flood RTC bus
    }
    SET_PERI_REG_MASK(RTC_CNTL_INT_CLR_REG, RTC_CNTL_TIME_VALID_INT_CLR);
    uint64_t t = READ_PERI_REG(RTC_CNTL_TIME0_REG);
    t |= ((uint64_t) READ_PERI_REG(RTC_CNTL_TIME1_REG)) << 32;
    return t;
}

static void RTC_IRAM_ATTR wake_stub()
{
    const uint32_t now = rtc_time_get();

    // We only look at rising edges
    if (PULSE_CNT_IS_LOW()) {
        ets_printf(no_rising_edge);
    } else {
        // Check to see if we should filter this edge out
        if ((now - last_detected_edge_us) >= filter_us) {
            //ets_printf("Filtered: %d, Last valid us: %d", (now - last_detected_edge_us), last_valid_edge_us);
            // Don't measure the first valid pulse (we need at least two pulses to measure the width)
            if (last_valid_edge_us != 0) {
                // pulse_width_us = (now - last_valid_edge_us);
            }
            s_pulse_count++;
            last_valid_edge_us = now;
            ets_printf(wake_pulse_count, s_pulse_count);
        }

        last_detected_edge_us = now;
    }

    if (s_pulse_count >= s_max_pulse_count) {
        ets_printf(max_reached);
        esp_default_wake_deep_sleep();
        esp_wake_deep_sleep();
        return;
    }

    while (REG_GET_FIELD(UART_STATUS_REG(0), UART_ST_UTX_OUT));
    REG_WRITE(RTC_ENTRY_ADDR_REG, (uint32_t)&wake_stub);
    // Go to sleep.
    CLEAR_PERI_REG_MASK(RTC_CNTL_STATE0_REG, RTC_CNTL_SLEEP_EN);
    SET_PERI_REG_MASK(RTC_CNTL_STATE0_REG, RTC_CNTL_SLEEP_EN);
    // A few CPU cycles may be necessary for the sleep to start...
    ets_delay_us(100);
    // never reaches here.
}