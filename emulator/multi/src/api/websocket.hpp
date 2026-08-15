#pragma once

#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <functional>
#include <vector>
#include <mutex>

namespace ecumult {

class WebSocketServer {
public:
    WebSocketServer(const std::string& host = "0.0.0.0",
                    uint16_t port = 8081,
                    int update_interval_ms = 100);
    ~WebSocketServer();

    bool start();
    void stop();
    bool isRunning() const;

    using WSFrameHandler = std::function<void(const std::string& client_id,
                                                const std::string& message)>;
    using WSUpdateGenerator = std::function<std::string()>;

    void setFrameHandler(WSFrameHandler handler);
    void setUpdateGenerator(WSUpdateGenerator generator);

private:
    void wsLoop();
    void broadcast(const std::string& message);

    std::string host_;
    uint16_t port_;
    int update_interval_ms_;
    std::atomic<bool> running_;
    std::thread ws_thread_;
    WSFrameHandler frame_handler_;
    WSUpdateGenerator update_gen_;

    std::vector<int> clients_;
    std::mutex client_mutex_;
};

} // namespace ecumult
