#ifndef _ISR_INTERNAL_GPIO_PIN_H_
#define _ISR_INTERNAL_GPIO_PIN_H_

#include <freertos/FreeRTOS.h>
#include <driver/gpio.h>

enum gpio {
    FLAG_PULLUP,
    FLAG_PULLDOWN,
    FLAG_INPUT
};

enum InterruptType {
    INTERRUPT_RISING_EDGE,
    INTERRUPT_FALLING_EDGE,
    INTERRUPT_ANY_EDGE,
    INTERRUPT_LOW_LEVEL,
    INTERRUPT_HIGH_LEVEL
};

class ISRInternalGPIOPin {
    public:
        ISRInternalGPIOPin(const gpio_num_t pin, const gpio_drive_cap_t drive_strength, const bool inverted, const uint32_t flags);
        //ISRInternalGPIOPin(const uint32_t pin, void *arg) : arg_(arg) {}
        
        void setup();
        bool digital_read();
        void digital_write(bool value);
        void attach_interrupt(void (*func)(void *), void *arg, InterruptType type) const;
        void detach_interrupt() const;
        bool digital_read_isr();
        //void pin_mode(gpio::Flags flags);

    protected:
        void *arg_ = nullptr;

    private:
        static bool isr_service_installed;

        const gpio_num_t pin;
        const uint32_t flags;
        const gpio_drive_cap_t drive_strength;
        const bool inverted;

        const gpio_mode_t flags_to_mode(const uint32_t flags);
};

#endif