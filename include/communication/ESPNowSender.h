#ifndef _ESPNOW_H
#define _ESPNOW_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <esp_now.h>
#include <string.h>
#include <stdio.h>

class ESPNowSender
{
private:
    esp_now_peer_info_t peerInfo;
    int channel;
    bool use_long_range;
    static bool is_initialized;
    

    void initialize_wifi();
    void initialize_espnow();

public:
    ESPNowSender(const uint8_t targetAddress[6], const int channel, const bool enable_long_range = false);

    bool connect();
    bool send_message(const uint8_t *message, const size_t size);
};

#endif