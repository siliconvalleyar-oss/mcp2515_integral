#ifndef HARDWARE_MCP2515_HPP
#define HARDWARE_MCP2515_HPP

#include <cstdint>
#include <vector>
#include <memory>
#include "hardware/spi.hpp"

namespace Hardware {

struct CANMessage {
    uint32_t id;
    bool extended;
    bool rtr;
    uint8_t dlc;
    uint8_t data[8];
};

namespace MCP2515Reg {
    constexpr uint8_t RXF0SIDH = 0x00;
    constexpr uint8_t RXF0SIDL = 0x01;
    constexpr uint8_t RXF0EID8 = 0x02;
    constexpr uint8_t RXF0EID0 = 0x03;
    constexpr uint8_t RXF1SIDH = 0x04;
    constexpr uint8_t RXF1SIDL = 0x05;
    constexpr uint8_t RXF1EID8 = 0x06;
    constexpr uint8_t RXF1EID0 = 0x07;
    constexpr uint8_t RXF2SIDH = 0x08;
    constexpr uint8_t RXF2SIDL = 0x09;
    constexpr uint8_t RXF2EID8 = 0x0A;
    constexpr uint8_t RXF2EID0 = 0x0B;
    constexpr uint8_t CANSTAT = 0x0E;
    constexpr uint8_t CANCTRL = 0x0F;
    constexpr uint8_t CNF1 = 0x2A;
    constexpr uint8_t CNF2 = 0x28;
    constexpr uint8_t CNF3 = 0x29;
    constexpr uint8_t RXF3SIDH = 0x10;
    constexpr uint8_t RXF3SIDL = 0x11;
    constexpr uint8_t RXF3EID8 = 0x12;
    constexpr uint8_t RXF3EID0 = 0x13;
    constexpr uint8_t RXF4SIDH = 0x14;
    constexpr uint8_t RXF4SIDL = 0x15;
    constexpr uint8_t RXF4EID8 = 0x16;
    constexpr uint8_t RXF4EID0 = 0x17;
    constexpr uint8_t RXF5SIDH = 0x18;
    constexpr uint8_t RXF5SIDL = 0x19;
    constexpr uint8_t RXF5EID8 = 0x1A;
    constexpr uint8_t RXF5EID0 = 0x1B;
    constexpr uint8_t TEC = 0x1C;
    constexpr uint8_t REC = 0x1D;
    constexpr uint8_t RXB0CTRL = 0x66;
    constexpr uint8_t RXB1CTRL = 0x76;
    constexpr uint8_t RXM0SIDH = 0x20;
    constexpr uint8_t RXM0SIDL = 0x21;
    constexpr uint8_t RXM0EID8 = 0x22;
    constexpr uint8_t RXM0EID0 = 0x23;
    constexpr uint8_t RXM1SIDH = 0x24;
    constexpr uint8_t RXM1SIDL = 0x25;
    constexpr uint8_t RXM1EID8 = 0x26;
    constexpr uint8_t RXM1EID0 = 0x27;
    constexpr uint8_t TXB0CTRL = 0x30;
    constexpr uint8_t TXB1CTRL = 0x40;
    constexpr uint8_t TXB2CTRL = 0x50;
    constexpr uint8_t TXB0SIDH = 0x31;
    constexpr uint8_t TXB1SIDH = 0x41;
    constexpr uint8_t TXB2SIDH = 0x51;
    constexpr uint8_t RXB0SIDH = 0x61;
    constexpr uint8_t RXB1SIDH = 0x71;
}

namespace MCP2515Cmd {
    constexpr uint8_t RESET = 0xC0;
    constexpr uint8_t READ = 0x03;
    constexpr uint8_t WRITE = 0x02;
    constexpr uint8_t RTS_TX0 = 0x81;
    constexpr uint8_t RTS_TX1 = 0x82;
    constexpr uint8_t RTS_TX2 = 0x84;
    constexpr uint8_t RTS_ALL = 0x87;
    constexpr uint8_t READ_STATUS = 0xA0;
    constexpr uint8_t RX_STATUS = 0xB0;
    constexpr uint8_t BIT_MODIFY = 0x05;
}

class MCP2515 {
public:
    enum class Bitrate {
        BPS_125K,
        BPS_250K,
        BPS_500K,
        BPS_1M
    };

    MCP2515(uint8_t csPin, std::shared_ptr<SPI> spi);
    ~MCP2515();

    bool initialize(Bitrate bitrate = Bitrate::BPS_500K);
    void cleanup();

    bool setBitrate(Bitrate bitrate);
    bool setFilter(uint8_t filterId, uint32_t id, bool extended);
    bool setMask(uint8_t maskId, uint32_t mask, bool extended);

    virtual bool sendMessage(const CANMessage& msg);
    bool sendMessage(uint32_t id, const uint8_t* data, uint8_t dlc, bool extended = false);

    virtual bool receiveMessage(CANMessage& msg);
    bool messageAvailable();

    uint8_t getErrorFlags();
    uint8_t getReceiveErrorCounter();
    uint8_t getTransmitErrorCounter();
    void reset();

private:
    uint8_t readRegister(uint8_t reg);
    void writeRegister(uint8_t reg, uint8_t value);
    void modifyRegister(uint8_t reg, uint8_t mask, uint8_t value);
    void setControlMode(uint8_t mode);

    uint8_t csPin_;
    std::shared_ptr<SPI> spi_;
    bool initialized_;
};

} // namespace Hardware

#endif // HARDWARE_MCP2515_HPP
