#pragma once

#include <cstdint>
#include <vector>
#include <queue>
#include <functional>

namespace ecumult {

class CANTransportProtocol {
public:
    // ISO 15765-2 (CAN-TP) states
    enum class State {
        IDLE,
        WAIT_SINGLE_FRAME,
        WAIT_FIRST_FRAME,
        RECEIVING_MULTI_FRAME,
        SENDING_MULTI_FRAME
    };

    // PCI types
    enum class PCI : uint8_t {
        SINGLE_FRAME      = 0x00,
        FIRST_FRAME       = 0x10,
        CONSECUTIVE_FRAME = 0x20,
        FLOW_CONTROL      = 0x30
    };

    // Flow control flags
    enum class FC : uint8_t {
        CONTINUE_TO_SEND   = 0x00,
        WAIT               = 0x01,
        OVERFLOW_ABORT     = 0x02
    };

    struct Message {
        std::vector<uint8_t> data;
        uint32_t source_id;
        uint32_t target_id;
        bool is_extended;
    };

    CANTransportProtocol();

    // Receive: process incoming CAN frame and return complete message if assembled
    std::vector<Message> processFrame(uint32_t can_id, const std::vector<uint8_t>& data);

    // Transmit: break message into CAN frames
    std::vector<std::pair<uint32_t, std::vector<uint8_t>>>
        prepareMessage(const Message& msg, uint16_t block_size = 0, uint8_t st_min = 0);

    void reset();
    State getState() const;

private:
    State state_;
    uint32_t current_source_id_;
    std::vector<uint8_t> receive_buffer_;
    uint16_t expected_total_size_;
    uint16_t received_size_;
    uint8_t expected_seq_num_;
    uint16_t block_size_;
    uint8_t st_min_;
    uint16_t frames_sent_;
    uint16_t total_frames_;
    uint32_t current_target_id_;
    bool is_extended_;

    uint8_t getPCIType(uint8_t first_byte);
    uint8_t getPCILength(uint8_t first_byte);
    uint16_t getFullLength(const std::vector<uint8_t>& data);

    std::vector<uint8_t> buildSingleFrame(const std::vector<uint8_t>& message);
    std::pair<std::vector<uint8_t>, std::vector<uint8_t>> buildFirstFrame(const std::vector<uint8_t>& message);
    std::vector<uint8_t> buildConsecutiveFrame(uint8_t seq_num, const std::vector<uint8_t>& data);
    std::vector<uint8_t> buildFlowControl(FC flag, uint8_t block_size, uint8_t st_min);
};

} // namespace ecumult
