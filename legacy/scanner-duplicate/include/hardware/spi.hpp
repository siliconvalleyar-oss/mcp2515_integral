#ifndef HARDWARE_SPI_HPP
#define HARDWARE_SPI_HPP

#include <cstdint>
#include <cstddef>
#include <string>

namespace Hardware {

class SPI {
public:
    static constexpr uint32_t DEFAULT_SPEED = 500000;  // 500 kHz
    static constexpr uint8_t DEFAULT_BITS = 8;
    static constexpr uint8_t DEFAULT_MODE = 0;

    SPI() = delete;
    SPI(const SPI&) = delete;
    SPI& operator=(const SPI&) = delete;

    explicit SPI(uint8_t channel, uint32_t speed = DEFAULT_SPEED,
                 uint8_t bits = DEFAULT_BITS, uint8_t mode = DEFAULT_MODE);

    ~SPI();

    bool initialize();
    void cleanup();

    uint8_t transfer(uint8_t data);
    void transfer(const uint8_t* tx, uint8_t* rx, size_t length);

    uint8_t channel() const { return channel_; }
    uint32_t speed() const { return speed_; }
    uint8_t bits() const { return bits_; }
    uint8_t mode() const { return mode_; }

private:
    uint8_t channel_;
    uint32_t speed_;
    uint8_t bits_;
    uint8_t mode_;
    int fd_;
};

} // namespace Hardware

#endif // HARDWARE_SPI_HPP
