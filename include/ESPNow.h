#ifndef _ESPNOW_H
#define _ESPNOW_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <esp_now.h>
#include <string.h>
#include <stdio.h>

class ESPNow
{
private:
    uint8_t pairAddress[6];
    static bool is_initialized;
    static void on_message_sent(const uint8_t *mac_addr, esp_now_send_status_t status);

public:
    ESPNow(const uint8_t pairAddress[6]);
    ~ESPNow();

    bool connect();
    bool send_message(const uint8_t *message, const size_t size);
};

#endif