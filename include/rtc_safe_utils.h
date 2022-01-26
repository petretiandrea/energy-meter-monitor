#include <freertos/FreeRTOS.h>
#include <rom/rtc.h>
#include <rom/ets_sys.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <soc/rtc_cntl_reg.h>
#include <soc/rtc_io_reg.h>
#include <soc/uart_reg.h>
#include <soc/timer_group_reg.h>
#include <sys/times.h>
#include "soc/frc_timer_reg.h"
#include "soc/rtc.h"

#define RTC_CNTL_SLOWCLK_FREQ 150000 //150kHz

static uint64_t RTC_IRAM_ATTR rtc_time_get_us()
{
    SET_PERI_REG_MASK(RTC_CNTL_TIME_UPDATE_REG, RTC_CNTL_TIME_UPDATE);
    while (GET_PERI_REG_MASK(RTC_CNTL_TIME_UPDATE_REG, RTC_CNTL_TIME_VALID) == 0) {
        ets_delay_us(1); // might take 1 RTC slowclk period, don't flood RTC bus
    }
    SET_PERI_REG_MASK(RTC_CNTL_INT_CLR_REG, RTC_CNTL_TIME_VALID_INT_CLR);
    uint64_t low = READ_PERI_REG(RTC_CNTL_TIME0_REG);
    uint64_t high = READ_PERI_REG(RTC_CNTL_TIME1_REG);
    uint64_t ticks = (high << 32) | low;
    return ticks * 100 / (RTC_CNTL_SLOWCLK_FREQ / 10000);
};  