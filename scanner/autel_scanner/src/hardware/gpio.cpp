#include "hardware/gpio.hpp"
#include <bcm2835.h>
#include <cerrno>
#include <cstring>
#include <functional>
#include <thread>
#include <chrono>
#include <atomic>
#include <unordered_map>
#include <memory>

namespace Hardware {

bool GPIO::initialized_ = false;

struct ISRData {
    std::function<void()> callback;
    GPIO::Edge edge;
    std::atomic<bool> running{false};
    std::thread thread;
};

static std::unordered_map<uint8_t, std::unique_ptr<ISRData>> isr_data_;

bool GPIO::initialize() {
    if (initialized_) return true;

    if (!bcm2835_init()) {
        return false;
    }
    initialized_ = true;
    return true;
}

void GPIO::cleanup() {
    if (initialized_) {
        for (auto& [pin, data] : isr_data_) {
            data->running.store(false);
            if (data->thread.joinable()) {
                data->thread.join();
            }
        }
        isr_data_.clear();
        bcm2835_close();
        initialized_ = false;
    }
}

void GPIO::setMode(uint8_t pin, PinMode mode) {
    uint8_t fsel = BCM2835_GPIO_FSEL_INPT;
    switch (mode) {
        case PinMode::INPUT:  fsel = BCM2835_GPIO_FSEL_INPT; break;
        case PinMode::OUTPUT: fsel = BCM2835_GPIO_FSEL_OUTP; break;
        case PinMode::ALT0:   fsel = BCM2835_GPIO_FSEL_ALT0; break;
        case PinMode::ALT1:   fsel = BCM2835_GPIO_FSEL_ALT1; break;
        case PinMode::ALT2:   fsel = BCM2835_GPIO_FSEL_ALT2; break;
        case PinMode::ALT3:   fsel = BCM2835_GPIO_FSEL_ALT3; break;
        case PinMode::ALT4:   fsel = BCM2835_GPIO_FSEL_ALT4; break;
        case PinMode::ALT5:   fsel = BCM2835_GPIO_FSEL_ALT5; break;
    }
    bcm2835_gpio_fsel(pin, fsel);
}

void GPIO::write(uint8_t pin, bool value) {
    bcm2835_gpio_write(pin, value ? HIGH : LOW);
}

bool GPIO::read(uint8_t pin) {
    return bcm2835_gpio_lev(pin) == HIGH;
}

void GPIO::setPullUpDown(uint8_t pin, PullUpDown pud) {
    uint8_t mode = BCM2835_GPIO_PUD_OFF;
    switch (pud) {
        case PullUpDown::PUD_DOWN: mode = BCM2835_GPIO_PUD_DOWN; break;
        case PullUpDown::PUD_UP:   mode = BCM2835_GPIO_PUD_UP; break;
        default: break;
    }
    bcm2835_gpio_set_pud(pin, mode);
}

void GPIO::setISR(uint8_t pin, Edge edge, std::function<void()> callback) {
    if (isr_data_.count(pin)) {
        isr_data_[pin]->running.store(false);
        if (isr_data_[pin]->thread.joinable()) {
            isr_data_[pin]->thread.join();
        }
        isr_data_.erase(pin);
    }

    auto data = std::make_unique<ISRData>();
    data->callback = std::move(callback);
    data->edge = edge;
    data->running.store(true);

    data->thread = std::thread([pin, data_ptr = data.get()]() {
        bool last_state = bcm2835_gpio_lev(pin);
        while (data_ptr->running.load()) {
            bool current_state = bcm2835_gpio_lev(pin);
            if (current_state != last_state) {
                bool should_trigger = false;
                switch (data_ptr->edge) {
                    case GPIO::Edge::RISING:
                        should_trigger = (current_state == HIGH);
                        break;
                    case GPIO::Edge::FALLING:
                        should_trigger = (current_state == LOW);
                        break;
                    case GPIO::Edge::BOTH:
                        should_trigger = true;
                        break;
                    default:
                        break;
                }
                if (should_trigger && data_ptr->callback) {
                    data_ptr->callback();
                }
                last_state = current_state;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    isr_data_[pin] = std::move(data);
}

} // namespace Hardware
