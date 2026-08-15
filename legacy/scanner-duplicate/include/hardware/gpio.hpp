#ifndef HARDWARE_GPIO_HPP
#define HARDWARE_GPIO_HPP

#include <cstdint>
#include <string>
#include <functional>

namespace Hardware {

class GPIO {
public:
    enum class PinMode {
        INPUT,
        OUTPUT,
        ALT0,
        ALT1,
        ALT2,
        ALT3,
        ALT4,
        ALT5
    };

    enum class PullUpDown {
        PUD_OFF,
        PUD_DOWN,
        PUD_UP
    };

    enum class Edge {
        NONE,
        RISING,
        FALLING,
        BOTH
    };

    GPIO() = delete;
    GPIO(const GPIO&) = delete;
    GPIO& operator=(const GPIO&) = delete;

    static bool initialize();
    static void cleanup();

    static void setMode(uint8_t pin, PinMode mode);
    static void write(uint8_t pin, bool value);
    static bool read(uint8_t pin);
    static void setPullUpDown(uint8_t pin, PullUpDown pud);

    static void setISR(uint8_t pin, Edge edge, std::function<void()> callback);

private:
    static bool initialized_;
};

} // namespace Hardware

#endif // HARDWARE_GPIO_HPP
