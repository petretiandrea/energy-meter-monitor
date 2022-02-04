
#include "communication/ESPNowSender.h"

#include <esp_err.h>
#include <esp_wifi.h>
#include <esp_netif.h>

bool ESPNowSender::is_initialized = false;

ESPNowSender::ESPNowSender(const uint8_t targetAddress[6], const int channel, const bool enable_long_range) : channel(channel), use_long_range(enable_long_range)
{
    memcpy(this->peerInfo.peer_addr, targetAddress, 6);
}

bool ESPNowSender::connect() {
    if (!is_initialized) {
        initialize_wifi();
        initialize_espnow();
        is_initialized = true;
    }
    return esp_now_add_peer(&peerInfo) == ESP_OK;
};

bool ESPNowSender::send_message(const uint8_t *message, const size_t size) {
    return esp_now_send(peerInfo.peer_addr, message, size) == ESP_OK;
};

void ESPNowSender::initialize_wifi() {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    cfg.nvs_enable = false;
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    if (this->use_long_range) {
        ESP_ERROR_CHECK(esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR));
    }

    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(this->channel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);
};

void ESPNowSender::initialize_espnow() {
    ESP_ERROR_CHECK(esp_now_init());
    this->peerInfo.channel = 0;
    this->peerInfo.encrypt = false;
    this->peerInfo.ifidx = WIFI_IF_STA;
}