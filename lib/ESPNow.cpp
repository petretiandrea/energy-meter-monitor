#include "ESPNow.h"

bool ESPNow::is_initialized = false;

ESPNow::ESPNow(const uint8_t pairAddress[6]) : pairAddress(pairAddress)
{
}

ESPNow::~ESPNow()
{
}

bool ESPNow::setup()
{
    if (!is_initialized) {
        esp_now_peer_info_t* peerInfo = new esp_now_peer_info_t();
        memcpy(peerInfo->peer_addr, pairAddress, 6);
        peerInfo->channel = 0;  
        peerInfo->encrypt = false;
        auto initialized = esp_now_init() == ESP_OK;
        initialized = initialized && esp_now_register_send_cb(on_message_sent) == ESP_OK;
        initialized = initialized && esp_now_add_peer(peerInfo) == ESP_OK;
        is_initialized = initialized;
    }
    return is_initialized;
}

bool ESPNow::send_message(const uint8_t *message, const size_t size) {
    auto result = esp_now_send(
        pairAddress,
        message,
        size
    );
    
}\