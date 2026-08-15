#pragma once

#include <string>
#include <memory>
#include <thread>
#include <atomic>

namespace ecumult {

class ProtocolRouter;
class DBManager;

class GRPCServer {
public:
    GRPCServer(std::shared_ptr<ProtocolRouter> router,
               std::shared_ptr<DBManager> db,
               const std::string& host = "0.0.0.0",
               uint16_t port = 50051);
    ~GRPCServer();

    bool start();
    void stop();
    bool isRunning() const;

private:
    std::shared_ptr<ProtocolRouter> router_;
    std::shared_ptr<DBManager> db_;
    std::string host_;
    uint16_t port_;
    std::atomic<bool> running_;
    std::thread server_thread_;
};

} // namespace ecumult
