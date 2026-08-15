#ifndef HARDWARE_I2C_HPP
#define HARDWARE_I2C_HPP

#include <cstdint>
#include <string>
#include <memory>

namespace Hardware {

class I2C {
public:
    static constexpr uint8_t DEFAULT_ADDRESS = 0x3C;
    static constexpr uint8_t DEFAULT_BUS = 1;

    I2C(uint8_t bus = DEFAULT_BUS, uint8_t address = DEFAULT_ADDRESS);
    ~I2C();

    bool initialize();
    void cleanup();

    bool write(const uint8_t* data, size_t length);
    bool write(uint8_t data);
    bool read(uint8_t* buffer, size_t length);
    bool readRegister(uint8_t reg, uint8_t* buffer, size_t length);
    bool writeRegister(uint8_t reg, uint8_t value);

    uint8_t address() const { return address_; }
    uint8_t bus() const { return bus_; }

private:
    uint8_t bus_;
    uint8_t address_;
    int fd_;
};

} // namespace Hardware

#endif // HARDWARE_I2C_HPP
