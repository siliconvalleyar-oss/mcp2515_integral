#ifndef MCP2515_H
#define MCP2515_H

#include <cstdint>
#include <cstddef>

class MCP2515 {
public:
    enum class Bitrate {
        KBPS125,
        KBPS250,
        KBPS500
    };

    enum class Mode : uint8_t {
        NORMAL = 0x00,
        LOOPBACK = 0x40,
        CONFIG = 0x80,
        LISTEN_ONLY = 0x60
    };

    struct CanFrame {
        uint32_t id = 0;
        uint8_t dlc = 0;
        uint8_t data[8] = {};
        bool extended = false;
    };

    MCP2515(int csPin, int intPin);

    bool begin(Bitrate bitrate = Bitrate::KBPS500);
    void end();

    bool setMode(Mode mode);

    bool send(const CanFrame& frame);
    bool receive(CanFrame& frame);

    bool available() const;

    void reset();

private:
    int csPin_;
    int intPin_;
    bool initialized_;

    void select();
    void deselect();

    uint8_t readRegister(uint8_t address);
    void readRegisters(uint8_t address, uint8_t* buffer, size_t len);

    void writeRegister(uint8_t address, uint8_t value);
    void writeRegisters(uint8_t address,
                        const uint8_t* buffer,
                        size_t len);

    void bitModify(uint8_t address, uint8_t mask, uint8_t data);

    uint8_t readStatus();

    bool configureBitrate(Bitrate bitrate);

    bool writeTxBuffer(const CanFrame& frame);
    bool requestToSend();

    bool readRxBuffer(uint8_t bufferNumber, CanFrame& frame);
};

#endif

