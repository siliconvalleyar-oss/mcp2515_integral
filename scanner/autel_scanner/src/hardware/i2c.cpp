#include "hardware/i2c.hpp"
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

namespace Hardware {

I2C::I2C(uint8_t bus, uint8_t address)
    : bus_(bus), address_(address), fd_(-1) {}

I2C::~I2C() {
    cleanup();
}

bool I2C::initialize() {
    if (fd_ >= 0) return true;

    std::string device = "/dev/i2c-" + std::to_string(bus_);
    fd_ = open(device.c_str(), O_RDWR);
    if (fd_ < 0) {
        return false;
    }

    if (ioctl(fd_, I2C_SLAVE, address_) < 0) {
        close(fd_);
        fd_ = -1;
        return false;
    }

    return true;
}

void I2C::cleanup() {
    if (fd_ >= 0) {
        close(fd_);
        fd_ = -1;
    }
}

bool I2C::write(const uint8_t* data, size_t length) {
    if (fd_ < 0 || length == 0) return false;

    ssize_t result = ::write(fd_, data, length);
    return result == static_cast<ssize_t>(length);
}

bool I2C::write(uint8_t data) {
    return write(&data, 1);
}

bool I2C::read(uint8_t* buffer, size_t length) {
    if (fd_ < 0 || length == 0) return false;

    ssize_t result = ::read(fd_, buffer, length);
    return result == static_cast<ssize_t>(length);
}

bool I2C::readRegister(uint8_t reg, uint8_t* buffer, size_t length) {
    if (fd_ < 0) return false;

    if (::write(fd_, &reg, 1) != 1) {
        return false;
    }

    return read(buffer, length);
}

bool I2C::writeRegister(uint8_t reg, uint8_t value) {
    if (fd_ < 0) return false;

    uint8_t buf[2] = {reg, value};
    return write(buf, 2);
}

} // namespace Hardware
