#include "grpc_server.hpp"
#include <iostream>

namespace ecumult {

GRPCServer::GRPCServer(std::shared_ptr<ProtocolRouter> router,
                       std::shared_ptr<DBManager> db,
                       const std::string& host, uint16_t port)
    : router_(std::move(router))
    , db_(std::move(db))
    , host_(host)
    , port_(port)
    , running_(false) {}

GRPCServer::~GRPCServer() {
    stop();
}

bool GRPCServer::start() {
    if (running_) return true;
    running_ = true;
    std::cout << "[gRPC] Server on " << host_ << ":" << port_ << " (requires gRPC library)" << std::endl;
    return true;
}

void GRPCServer::stop() {
    running_ = false;
    if (server_thread_.joinable()) server_thread_.join();
}

bool GRPCServer::isRunning() const {
    return running_;
}

} // namespace ecumult
