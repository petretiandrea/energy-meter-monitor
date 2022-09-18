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
#include "pulse_meter/PulseCounter.h"
#include "rtc_safe_utils.h"
#include <esp_system.h>
#include <esp_now.h>
#include <string.h>
#include <driver/rtc_io.h>

#include "RfCalibration.h"
#include "communication/ESPNowSender.h"
#include "model/PulseCounterData.h"
#include "model/EnergyMeterMeasure.h"
#include "protocol/edge/Protocol.h"

#include "sdkconfig.h"

extern "C" void app_main(void);

const RTC_DATA_ATTR uint64_t WAKE_UP_TIME = CONFIG_WAKE_UP_TIME * 1000 * 1000; // 30s

// Pulse counter value, stored in RTC_SLOW_MEM
static uint64_t RTC_DATA_ATTR last_wake_time_us;
static PulseCounterConfig RTC_DATA_ATTR pulse_config {
    .gpio_num = (gpio_num_t) CONFIG_PULSE_GPIO_NUM,
    .drive_cap = GPIO_DRIVE_CAP_0,
    .filter_us = CONFIG_PULSE_INTERNAL_FILTER_US,
    .pulse_timeout_us = CONFIG_PULSE_TIMEOUT_US
};
static PulseCounterData RTC_DATA_ATTR data;

//uint8_t peer[] = { 0x4a, 0x3f, 0xda, 0x0d, 0xbe, 0xb0 };
uint8_t peer[] = { 0x4a, 0x3f, 0xda, 0x0d, 0xbb, 0xce };

#ifdef CONFIG_USE_LONG_RANGE
    ESPNowSender sender(peer, 0, CONFIG_USE_LONG_RANGE);
#else
    ESPNowSender sender(peer, 0 , false);
#endif

// Function which runs after exit from deep sleep
static void RTC_IRAM_ATTR wake_stub_pulse_counter();

void app_main(void) {

    // First of all setup pulse counter and attach interrupt. 
    // This avoid loosing pulse while chip is wake.
    PulseCounter::setup(&pulse_config);
    PulseCounter::attach_intterupt(&pulse_config, &data);

    auto wakeup_reason = rtc_get_reset_reason(0);
    if (wakeup_reason != DEEPSLEEP_RESET) {
        printf("First boot...setting up initialization phase\n");
        last_wake_time_us = 0;
        data.initialize();
        calibrate_rf();

        // send announcement
        protocol::edge::ProtocolMessage msg = {
            .is_announce = true,
            .type = protocol::edge::DeviceType::POWER_METER_PULSES,
            .announce = {
                {.device_name = CONFIG_DEVICE_NAME}
            },
        };
        if (sender.connect()) {
            sender.send_message((uint8_t*) &msg, sizeof(msg));
            vTaskDelay(500 / portTICK_PERIOD_MS); // litte delay to send before sleep
            printf("Sended\n");
        }
    } else {
        printf("Wake up from deep sleep\n");
        auto toSend = parser::toDataMessage(EnergyMeterMeasureFactory::createBy(&data, CONFIG_PULSE_RATE));
        protocol::edge::ProtocolMessage msg = {
            .is_announce = false,
            .type = protocol::edge::DeviceType::POWER_METER_PULSES,
            .data = *toSend,
        };
        restore_calibration_from_rtc();
        if (sender.connect()) {
            sender.send_message((uint8_t*) &msg, sizeof(msg));
            printf("Sended measures\n");
        }
        delete toSend;
    }

    printf("Going to deep...\n");
    // reatain gpio pad and Enter deep sleep
    // gpio_hold_en(pulse_config.gpio_num);
    // gpio_deep_sleep_hold_en();
    // esp_set_deep_sleep_wake_stub(&wake_stub_pulse_counter);
    // esp_sleep_enable_ext1_wakeup(1ULL << static_cast<uint32_t>(pulse_config.gpio_num), ESP_EXT1_WAKEUP_ANY_HIGH);
    
    // rtc_gpio_isolate(GPIO_NUM_12);
    // rtc_gpio_isolate(GPIO_NUM_15);
    // esp_deep_sleep_disable_rom_logging(); // suppress boot messages
    // // set last wake time before go to sleep. Assigned here automatically exclude the time spent while is wake.
    // esp_wifi_stop();
    // printf("Going to deep2...\n");
    // last_wake_time_us = rtc_time_get_us();
    // esp_deep_sleep_start();
};

static void RTC_IRAM_ATTR wake_stub_pulse_counter()
{
    const uint64_t now = rtc_time_get_us();

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