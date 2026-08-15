#include "can_tp.hpp"
#include <cstring>
#include <algorithm>

namespace ecumult {

CANTransportProtocol::CANTransportProtocol()
    : state_(State::IDLE)
    , current_source_id_(0)
    , expected_total_size_(0)
    , received_size_(0)
    , expected_seq_num_(1)
    , block_size_(0)
    , st_min_(0)
    , frames_sent_(0)
    , total_frames_(0)
    , current_target_id_(0)
    , is_extended_(false) {}

std::vector<CANTransportProtocol::Message> CANTransportProtocol::processFrame(
    uint32_t can_id, const std::vector<uint8_t>& data) {
    std::vector<Message> completed;
    if (data.empty()) return completed;

    uint8_t pci_type = data[0] & 0xF0;
    uint8_t pci_len = data[0] & 0x0F;

    switch (static_cast<PCI>(pci_type)) {
        case PCI::SINGLE_FRAME: {
            Message msg;
            msg.source_id = can_id;
            // data is from byte 1, length is pci_len
            if (data.size() > 1) {
                msg.data.assign(data.begin() + 1, data.begin() + 1 + std::min((size_t)pci_len, data.size() - 1));
            }
            completed.push_back(msg);
            break;
        }
        case PCI::FIRST_FRAME: {
            if (data.size() < 2) break;
            uint16_t total_len = ((data[0] & 0x0F) << 8) | data[1];
            receive_buffer_.clear();
            receive_buffer_.reserve(total_len);
            expected_total_size_ = total_len;
            received_size_ = 0;
            expected_seq_num_ = 2;
            current_source_id_ = can_id;

            // First frame may contain initial data
            if (data.size() > 2) {
                size_t init_data = std::min(data.size() - 2, (size_t)total_len);
                receive_buffer_.insert(receive_buffer_.end(),
                                       data.begin() + 2, data.begin() + 2 + init_data);
                received_size_ += init_data;
            }
            state_ = State::RECEIVING_MULTI_FRAME;
            break;
        }
        case PCI::CONSECUTIVE_FRAME: {
            if (state_ != State::RECEIVING_MULTI_FRAME || can_id != current_source_id_) break;
            uint8_t seq_num = data[0] & 0x0F;
            if (seq_num != expected_seq_num_) {
                state_ = State::IDLE;
                break;
            }
            expected_seq_num_ = (seq_num == 0x0F) ? 0x00 : seq_num + 1;
            size_t remaining = expected_total_size_ - received_size_;
            size_t to_copy = std::min(remaining, data.size() - 1);
            receive_buffer_.insert(receive_buffer_.end(),
                                   data.begin() + 1, data.begin() + 1 + to_copy);
            received_size_ += to_copy;

            if (received_size_ >= expected_total_size_) {
                Message msg;
                msg.source_id = current_source_id_;
                msg.data.swap(receive_buffer_);
                completed.push_back(msg);
                state_ = State::IDLE;
            }
            break;
        }
        case PCI::FLOW_CONTROL: {
            // Handle flow control for outgoing messages
            if (state_ != State::SENDING_MULTI_FRAME) break;
            FC flag = static_cast<FC>(data[0] & 0x0F);
            if (flag == FC::CONTINUE_TO_SEND) {
                // Continue sending remaining frames
            } else if (flag == FC::OVERFLOW_ABORT) {
                state_ = State::IDLE;
            }
            break;
        }
    }
    return completed;
}

std::vector<std::pair<uint32_t, std::vector<uint8_t>>>
    CANTransportProtocol::prepareMessage(const Message& msg, uint16_t block_size, uint8_t st_min) {
    std::vector<std::pair<uint32_t, std::vector<uint8_t>>> frames;
    (void)block_size;
    (void)st_min;

    uint32_t target_id = msg.target_id;

    if (msg.data.size() <= 7) {
        // Single frame
        auto sf = buildSingleFrame(msg.data);
        frames.emplace_back(target_id, sf);
    } else {
        // Multi-frame
        auto [ff_data, remaining] = buildFirstFrame(msg.data);
        frames.emplace_back(target_id, ff_data);

        uint8_t seq_num = 2;
        while (!remaining.empty()) {
            size_t chunk = std::min(remaining.size(), size_t(7));
            std::vector<uint8_t> chunk_data(remaining.begin(), remaining.begin() + chunk);
            auto cf = buildConsecutiveFrame(seq_num, chunk_data);
            frames.emplace_back(target_id, cf);
            remaining.erase(remaining.begin(), remaining.begin() + chunk);
            seq_num = (seq_num == 0x0F) ? 0x00 : seq_num + 1;
        }
    }
    return frames;
}

void CANTransportProtocol::reset() {
    state_ = State::IDLE;
    receive_buffer_.clear();
    expected_total_size_ = 0;
    received_size_ = 0;
    expected_seq_num_ = 0;
}

CANTransportProtocol::State CANTransportProtocol::getState() const {
    return state_;
}

uint8_t CANTransportProtocol::getPCIType(uint8_t first_byte) {
    return first_byte & 0xF0;
}

uint8_t CANTransportProtocol::getPCILength(uint8_t first_byte) {
    return first_byte & 0x0F;
}

uint16_t CANTransportProtocol::getFullLength(const std::vector<uint8_t>& data) {
    if (data.size() < 2) return 0;
    return ((data[0] & 0x0F) << 8) | data[1];
}

std::vector<uint8_t> CANTransportProtocol::buildSingleFrame(const std::vector<uint8_t>& message) {
    std::vector<uint8_t> frame;
    uint8_t len = std::min(message.size(), size_t(7));
    frame.push_back(static_cast<uint8_t>(PCI::SINGLE_FRAME) | len);
    frame.insert(frame.end(), message.begin(), message.begin() + len);
    return frame;
}

std::pair<std::vector<uint8_t>, std::vector<uint8_t>>
    CANTransportProtocol::buildFirstFrame(const std::vector<uint8_t>& message) {
    uint16_t total_len = message.size();
    std::vector<uint8_t> ff;
    ff.push_back(static_cast<uint8_t>(PCI::FIRST_FRAME) | ((total_len >> 8) & 0x0F));
    ff.push_back(total_len & 0xFF);

    size_t init_data = std::min(message.size(), size_t(6));
    ff.insert(ff.end(), message.begin(), message.begin() + init_data);

    std::vector<uint8_t> remaining(message.begin() + init_data, message.end());
    return {ff, remaining};
}

std::vector<uint8_t> CANTransportProtocol::buildConsecutiveFrame(uint8_t seq_num,
                                                                    const std::vector<uint8_t>& data) {
    std::vector<uint8_t> cf;
    cf.push_back(static_cast<uint8_t>(PCI::CONSECUTIVE_FRAME) | (seq_num & 0x0F));
    size_t chunk = std::min(data.size(), size_t(7));
    cf.insert(cf.end(), data.begin(), data.begin() + chunk);
    return cf;
}

std::vector<uint8_t> CANTransportProtocol::buildFlowControl(FC flag, uint8_t block_size, uint8_t st_min) {
    return {static_cast<uint8_t>(PCI::FLOW_CONTROL) | static_cast<uint8_t>(flag), block_size, st_min};
}

} // namespace ecumult
