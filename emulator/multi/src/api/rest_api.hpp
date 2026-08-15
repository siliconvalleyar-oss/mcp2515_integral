#pragma once

#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <functional>
#include <unordered_map>

namespace ecumult {

class ProtocolRouter;
class DBManager;

class RESTAPI {
public:
    RESTAPI(std::shared_ptr<ProtocolRouter> router,
            std::shared_ptr<DBManager> db,
            const std::string& host = "0.0.0.0",
            uint16_t port = 8080);
    ~RESTAPI();

    bool start();
    void stop();
    bool isRunning() const;

    using RouteHandler = std::function<std::string(const std::string& path,
                                                     const std::string& method,
                                                     const std::string& body,
                                                     const std::unordered_map<std::string, std::string>& params)>;

    void setRouteHandler(RouteHandler handler);

private:
    void apiLoop();

    std::shared_ptr<ProtocolRouter> router_;
    std::shared_ptr<DBManager> db_;
    std::string host_;
    uint16_t port_;
    std::atomic<bool> running_;
    std::thread server_thread_;
    RouteHandler custom_handler_;
};

} // namespace ecumult
