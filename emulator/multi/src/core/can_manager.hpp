#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <linux/can.h>
#include <linux/can/raw.h>

namespace ecumult {

enum class CANState {
    DOWN,
    UP,
    ERROR
};

struct CANFrame {
    can_frame frame;
    struct timespec timestamp;
    uint32_t bus_id;
};

class CANManager {
public:
    using FrameCallback = std::function<void(const CANFrame&)>;

    CANManager(const std::string& interface_name = "can0",
               int bitrate = 500000);
    ~CANManager();

    bool start();
    void stop();
    bool sendFrame(const can_frame& frame);
    bool sendFrame(uint32_t id, const std::vector<uint8_t>& data, bool is_extended = false);
    void registerCallback(uint32_t can_id, uint32_t mask, FrameCallback cb);
    void unregisterCallback(uint32_t can_id);
    CANState getState() const;
    std::string getInterfaceName() const;
    void setBitrate(int bitrate);

private:
    void rxLoop();
    void txLoop();
    int openSocket();
    bool configureInterface();

    std::string interface_name_;
    int bitrate_;
    int socket_;
    std::atomic<CANState> state_;
    std::thread rx_thread_;
    std::thread tx_thread_;
    std::atomic<bool> running_;

    struct FilterEntry {
        uint32_t can_id;
        uint32_t mask;
        FrameCallback callback;
    };
    std::vector<FilterEntry> filters_;
    mutable std::mutex filter_mutex_;
    mutable std::mutex tx_mutex_;
    std::vector<can_frame> tx_queue_;
};

} // namespace ecumult
