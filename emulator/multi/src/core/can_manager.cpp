#include "can_manager.hpp"
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <fcntl.h>
#include <poll.h>

namespace ecumult {

CANManager::CANManager(const std::string& interface_name, int bitrate)
    : interface_name_(interface_name)
    , bitrate_(bitrate)
    , socket_(-1)
    , state_(CANState::DOWN)
    , running_(false) {}

CANManager::~CANManager() {
    stop();
}

bool CANManager::start() {
    if (state_ == CANState::UP) return true;

    socket_ = openSocket();
    if (socket_ < 0) {
        state_ = CANState::ERROR;
        return false;
    }

    if (!configureInterface()) {
        ::close(socket_);
        socket_ = -1;
        state_ = CANState::ERROR;
        return false;
    }

    running_ = true;
    state_ = CANState::UP;
    rx_thread_ = std::thread(&CANManager::rxLoop, this);
    tx_thread_ = std::thread(&CANManager::txLoop, this);
    return true;
}

void CANManager::stop() {
    running_ = false;
    state_ = CANState::DOWN;
    if (rx_thread_.joinable()) rx_thread_.join();
    if (tx_thread_.joinable()) tx_thread_.join();
    if (socket_ >= 0) {
        ::close(socket_);
        socket_ = -1;
    }
}

bool CANManager::sendFrame(const can_frame& frame) {
    std::lock_guard<std::mutex> lock(tx_mutex_);
    tx_queue_.push_back(frame);
    return true;
}

bool CANManager::sendFrame(uint32_t id, const std::vector<uint8_t>& data, bool is_extended) {
    can_frame frame{};
    frame.can_id = id;
    if (is_extended) frame.can_id |= CAN_EFF_FLAG;
    frame.can_dlc = std::min(data.size(), size_t(8));
    std::memcpy(frame.data, data.data(), frame.can_dlc);
    return sendFrame(frame);
}

void CANManager::registerCallback(uint32_t can_id, uint32_t mask, FrameCallback cb) {
    std::lock_guard<std::mutex> lock(filter_mutex_);
    filters_.push_back({can_id, mask, std::move(cb)});
}

void CANManager::unregisterCallback(uint32_t can_id) {
    std::lock_guard<std::mutex> lock(filter_mutex_);
    filters_.erase(
        std::remove_if(filters_.begin(), filters_.end(),
            [can_id](const FilterEntry& e) { return e.can_id == can_id; }),
        filters_.end());
}

CANState CANManager::getState() const {
    return state_.load();
}

std::string CANManager::getInterfaceName() const {
    return interface_name_;
}

void CANManager::setBitrate(int bitrate) {
    bitrate_ = bitrate;
}

int CANManager::openSocket() {
    int s = ::socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (s < 0) {
        std::cerr << "[CAN] Error opening socket: " << strerror(errno) << std::endl;
        return -1;
    }

    int on = 1;
    if (::setsockopt(s, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS, &on, sizeof(on)) < 0) {
        std::cerr << "[CAN] Error setting recv_own_msgs: " << strerror(errno) << std::endl;
    }

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    if (::setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        std::cerr << "[CAN] Error setting timeout: " << strerror(errno) << std::endl;
    }

    return s;
}

bool CANManager::configureInterface() {
    struct ifreq ifr{};
    std::strncpy(ifr.ifr_name, interface_name_.c_str(), IFNAMSIZ - 1);

    struct sockaddr_can addr{};
    addr.can_family = AF_CAN;

    if (::ioctl(socket_, SIOCGIFINDEX, &ifr) < 0) {
        std::cerr << "[CAN] Error getting interface index: " << strerror(errno) << std::endl;
        return false;
    }
    addr.can_ifindex = ifr.ifr_ifindex;

    if (::bind(socket_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[CAN] Error binding socket: " << strerror(errno) << std::endl;
        return false;
    }

    return true;
}

void CANManager::rxLoop() {
    std::cout << "[CAN] RX thread started on " << interface_name_ << std::endl;

    struct pollfd pfd;
    pfd.fd = socket_;
    pfd.events = POLLIN;

    while (running_) {
        int ret = ::poll(&pfd, 1, 100);
        if (ret < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (ret == 0) continue;

        can_frame frame{};
        struct sockaddr_can addr;
        socklen_t addrlen = sizeof(addr);

        int nbytes = ::recvfrom(socket_, &frame, sizeof(can_frame),
                                0, (struct sockaddr*)&addr, &addrlen);
        if (nbytes < 0) {
            if (errno == EINTR) continue;
            break;
        }

        CANFrame cf;
        cf.frame = frame;
        cf.bus_id = addr.can_ifindex;
        ::clock_gettime(CLOCK_MONOTONIC, &cf.timestamp);

        std::lock_guard<std::mutex> lock(filter_mutex_);
        for (const auto& entry : filters_) {
            if ((frame.can_id & entry.mask) == (entry.can_id & entry.mask)) {
                if (entry.callback) {
                    entry.callback(cf);
                }
            }
        }
    }
    std::cout << "[CAN] RX thread stopped" << std::endl;
}

void CANManager::txLoop() {
    while (running_) {
        can_frame frame{};
        {
            std::lock_guard<std::mutex> lock(tx_mutex_);
            if (tx_queue_.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            frame = tx_queue_.front();
            tx_queue_.erase(tx_queue_.begin());
        }

        int nbytes = ::write(socket_, &frame, sizeof(can_frame));
        if (nbytes < 0) {
            std::cerr << "[CAN] TX error: " << strerror(errno) << std::endl;
        }
    }
}

} // namespace ecumult
