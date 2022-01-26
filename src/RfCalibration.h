#include <phy.h>
#include <esp_attr.h>
#include <esp_phy_init.h>

/**
 * @file RfCalibration.h
 * @author Andrea Petreti 
 * @brief Allow to save and retrieve the rf calibration data from the RTC memory. In this
 * way the calibration is preserved during deep sleep phase, and can be retrieved when wake up without NVS initialization.
 * @version 0.1
 * @date 2022-01-26
 * @copyright Copyright (c) 2022
 */

static RTC_DATA_ATTR esp_phy_calibration_data_t rtc_cal_data;

/**
 * @brief Perform the calibration of the RF chip and save it in the RTC memory.
 */
void calibrate_rf() {
    const esp_phy_init_data_t* init_data = esp_phy_get_init_data();
    if (init_data == NULL) {
        printf("Failed to obtain PHY init data");
        abort();
    }

    esp_phy_enable();

    // Full calibration on reset, store calibration data in RTC memory.
    register_chipv7_phy(init_data, &rtc_cal_data, PHY_RF_CAL_FULL);
}

void restore_calibration_from_rtc() {
    esp_phy_common_clock_enable();
    const esp_phy_init_data_t* init_data = esp_phy_get_init_data();
    register_chipv7_phy(init_data, &rtc_cal_data, PHY_RF_CAL_NONE);

    phy_wakeup_init();
}