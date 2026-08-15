#include "hardware/spi.hpp"
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

namespace Hardware {

SPI::SPI(uint8_t channel, uint32_t speed, uint8_t bits, uint8_t mode)
    : channel_(channel), speed_(speed), bits_(bits), mode_(mode), fd_(-1) {}

SPI::~SPI() {
    cleanup();
}

bool SPI::initialize() {
    if (fd_ >= 0) return true;

    std::string device = "/dev/spidev0." + std::to_string(channel_);

    fd_ = open(device.c_str(), O_RDWR);
    if (fd_ < 0) {
        return false;
    }

    if (ioctl(fd_, SPI_IOC_WR_MODE, &mode_) < 0) {
        close(fd_);
        fd_ = -1;
        return false;
    }

    if (ioctl(fd_, SPI_IOC_WR_BITS_PER_WORD, &bits_) < 0) {
        close(fd_);
        fd_ = -1;
        return false;
    }

    if (ioctl(fd_, SPI_IOC_WR_MAX_SPEED_HZ, &speed_) < 0) {
        close(fd_);
        fd_ = -1;
        return false;
    }

    return true;
}

void SPI::cleanup() {
    if (fd_ >= 0) {
        close(fd_);
        fd_ = -1;
    }
}

uint8_t SPI::transfer(uint8_t data) {
    if (fd_ < 0) return 0;

    uint8_t rx = 0;
    struct spi_ioc_transfer tr;
    memset(&tr, 0, sizeof(tr));
    tr.tx_buf = reinterpret_cast<unsigned long>(&data);
    tr.rx_buf = reinterpret_cast<unsigned long>(&rx);
    tr.len = 1;
    tr.speed_hz = speed_;
    tr.delay_usecs = 0;
    tr.bits_per_word = bits_;
    tr.cs_change = 0;
    tr.tx_nbits = 0;
    tr.rx_nbits = 0;
    tr.word_delay_usecs = 0;

    ioctl(fd_, SPI_IOC_MESSAGE(1), &tr);
    return rx;
}

void SPI::transfer(const uint8_t* tx, uint8_t* rx, size_t length) {
    if (fd_ < 0 || length == 0) return;

    struct spi_ioc_transfer tr;
    memset(&tr, 0, sizeof(tr));
    tr.tx_buf = reinterpret_cast<unsigned long>(tx);
    tr.rx_buf = reinterpret_cast<unsigned long>(rx);
    tr.len = static_cast<uint32_t>(length);
    tr.speed_hz = speed_;
    tr.delay_usecs = 0;
    tr.bits_per_word = bits_;
    tr.cs_change = 0;
    tr.tx_nbits = 0;
    tr.rx_nbits = 0;
    tr.word_delay_usecs = 0;

    ioctl(fd_, SPI_IOC_MESSAGE(1), &tr);
}

} // namespace Hardware
