#include "ISRInternalGPIOPin.h"
#include <iostream>

using namespace std;

bool ISRInternalGPIOPin::isr_service_installed = false;

ISRInternalGPIOPin::ISRInternalGPIOPin(
    const gpio_num_t pin, 
    const gpio_drive_cap_t drive_strength, 
    const bool inverted, 
    const uint32_t flags
) : pin(pin), flags(flags), drive_strength(drive_strength), inverted(inverted) {
    isr_service_installed = false;
};

const gpio_mode_t ISRInternalGPIOPin::flags_to_mode(const uint32_t flags) {
    if (flags == FLAG_INPUT) {
        return GPIO_MODE_INPUT;
    } else {
        return GPIO_MODE_DISABLE;
    }
}

void ISRInternalGPIOPin::setup() {
    gpio_config_t conf{};
    conf.pin_bit_mask = 1ULL << static_cast<uint32_t>(pin);
    conf.mode = flags_to_mode(flags);
    conf.pull_up_en = flags & gpio::FLAG_PULLUP ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
    conf.pull_down_en = flags & gpio::FLAG_PULLDOWN ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;
    conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&conf);
    gpio_set_drive_capability(pin, drive_strength);
}

void ISRInternalGPIOPin::attach_interrupt(void (*func)(void *), void *arg, InterruptType type) const {
    gpio_int_type_t idf_type = GPIO_INTR_ANYEDGE;
    switch (type) {
        case InterruptType::INTERRUPT_RISING_EDGE:
            idf_type = inverted ? GPIO_INTR_NEGEDGE : GPIO_INTR_POSEDGE;
        break;
        case InterruptType::INTERRUPT_FALLING_EDGE:
            idf_type = inverted ? GPIO_INTR_POSEDGE : GPIO_INTR_NEGEDGE;
        break;
        case InterruptType::INTERRUPT_ANY_EDGE:
            idf_type = GPIO_INTR_ANYEDGE;
        break;
        case InterruptType::INTERRUPT_LOW_LEVEL:
            idf_type = inverted ? GPIO_INTR_HIGH_LEVEL : GPIO_INTR_LOW_LEVEL;
        break;
        case InterruptType::INTERRUPT_HIGH_LEVEL:
            idf_type = inverted ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL;
        break;
    }
    gpio_set_intr_type(pin, idf_type);
    gpio_intr_enable(pin);
    if (!isr_service_installed) {
        auto res = gpio_install_isr_service(ESP_INTR_FLAG_LEVEL3);
        if (res != ESP_OK) {
            cout << "attach_interrupt(): call to gpio_install_isr_service() failed, error code " << res << endl;
            return;
        }
        isr_service_installed = true;
    }
    gpio_isr_handler_add(pin, func, arg);
}

bool ISRInternalGPIOPin::digital_read() { return bool(gpio_get_level(pin)) != inverted; }
void ISRInternalGPIOPin::digital_write(bool value) { gpio_set_level(pin, value != inverted ? 1 : 0); }
void ISRInternalGPIOPin::detach_interrupt() const { gpio_intr_disable(pin); }


bool IRAM_ATTR ISRInternalGPIOPin::digital_read_isr() {
    auto *arg = reinterpret_cast<ISRInternalGPIOPin *>(this);
    return bool(gpio_get_level(arg->pin)) != arg->inverted;
}
// void IRAM_ATTR ISRInternalGPIOPin::digital_write(bool value) {
//     auto *arg = reinterpret_cast<ISRInternalGPIOPin *>(arg_);
//     gpio_set_level(arg->pin, value != arg->inverted ? 1 : 0);
// }