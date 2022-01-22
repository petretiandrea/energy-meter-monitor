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
#include "PulseMeter.h"

extern "C" void app_main(void);

const int INTERNAL_FILTER = 100; // ms
const gpio_num_t GPIO_LED = GPIO_NUM_33;
const adc_atten_t ATTENUATION = ADC_ATTEN_DB_11; // max input 2.2v
const adc1_channel_t PHOTORESISTOR_PIN = ADC1_CHANNEL_4;
const adc_bits_width_t PHOTORESISTOR_PRECISION = ADC_WIDTH_BIT_12;

PulseMeter pulseMeter;

void scan_wifi();

void blink_task(void* parameters) {
    while(1) {
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        gpio_set_level(GPIO_LED, 1);
        vTaskDelay(150 / portTICK_PERIOD_MS);
        gpio_set_level(GPIO_LED, 0);
    }
}

void app_main(void) {
    
    gpio_reset_pin(GPIO_LED);
    gpio_set_direction(GPIO_LED, GPIO_MODE_OUTPUT);

    xTaskCreate(
        blink_task,
        "blink_task",
        2048,
        NULL,
        5,
        NULL
    );

    scan_wifi();
    pulseMeter.setup();

    while (1) {
        pulseMeter.update();
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    // adc1_config_width(PHOTORESISTOR_PRECISION);
    // adc1_config_channel_atten(PHOTORESISTOR_PIN, ATTENUATION);
    
    // gpio_reset_pin(GPIO_LED);
    // gpio_set_direction(GPIO_LED, GPIO_MODE_OUTPUT);

    // int64_t last_valid_edge_us = 0;
    // int64_t timeout_us = 0;
    
    // int64_t pulse_width_us = 0;

    // while(1) {
    //     const int64_t now = esp_timer_get_time();
    //     const int64_t since_last_edge_us = now - last_valid_edge_us; // microseconds

    //     if (last_valid_edge_us != 0 && since_last_edge_us > timeout_us) {
    //         std::cout << "No pulse detected" << std::endl;
    //         pulse_width_us = 0;
    //         last_valid_edge_us = 0;
    //     }

    //     int value = adc1_get_raw(PHOTORESISTOR_PIN);
    //     std::cout << "Value: " << value << std::endl;
    //     if (value > 2000) {
    //         gpio_set_level(GPIO_LED, 1);
    //         vTaskDelay(1000 / portTICK_PERIOD_MS);
    //     }
    //     gpio_set_level(GPIO_LED, 0);
    //     vTaskDelay(2000 / portTICK_PERIOD_MS);
    // }

    vTaskDelay(50 * 1000 / portTICK_PERIOD_MS);
};

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    return ESP_OK;
}

static char* getAuthModeName(wifi_auth_mode_t auth_mode) {
	
	char *names[] = {"OPEN", "WEP", "WPA PSK", "WPA2 PSK", "WPA WPA2 PSK", "MAX"};
	return names[auth_mode];
}

void scan_wifi() {
    ESP_ERROR_CHECK(nvs_flash_init());

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&wifi_config));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_start());

    // configure and run the scan process in blocking mode
	wifi_scan_config_t scan_config = {
		.ssid = 0,
		.bssid = 0,
		.channel = 0,
        .show_hidden = true
    };

    printf("Start scanning...");
	ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));
	printf(" completed!\n");
	printf("\n");

    uint16_t ap_num = 20;
	wifi_ap_record_t ap_records[20];
	ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_num, ap_records));
	
	// print the list 
	printf("Found %d access points:\n", ap_num);
	printf("\n");
	printf("               SSID              | Channel | RSSI |   Auth Mode \n");
	printf("----------------------------------------------------------------\n");
	for(int i = 0; i < ap_num; i++)
		printf("%32s | %7d | %4d | %12s\n", (char *)ap_records[i].ssid, ap_records[i].primary, ap_records[i].rssi, getAuthModeName(ap_records[i].authmode));
	printf("----------------------------------------------------------------\n");
}