

## Improvements
[] Use a `throttle_average` like EPSHome
[x] Optimize deep sleep wake (by starting ESPNOW) check how to start EPSNow withou NVS and stroging PHY data to RTC memory
[] Use ULP to count
[x] Set ESPNow long range (use a custom define flag)
[] Use Deduplicator to avoid sending EPSNow data if not necessary
```c
// #if CONFIG_ESPNOW_ENABLE_LONG_RANGE
    ESP_ERROR_CHECK( esp_wifi_set_protocol(ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR) );
// #endif
```

### Improvingin power consumption and time-to-wake
Inspired from [https://github.com/FruitieX/esp32-volume-control-sender](https://github.com/FruitieX/esp32-volume-control-sender)
- RF Calibration at first boot and saved to RTC memory. Each wake from deep sleep load calibration from RTC memory.
- Disabled `CONFIG_ESP32_WIFI_NVS_ENABLED`, `CONFIG_ESP32_PHY_CALIBRATION_AND_DATA_STORAGE`. 
- ESP32 WROOVER rtc_gpio_isolate(GPIO_NUM_12) and rtc_gpio_isolate(GPIO_NUM_15);. Save power