#include "websocket.hpp"
#include <iostream>
#include <unistd.h>

namespace ecumult {

WebSocketServer::WebSocketServer(const std::string& host, uint16_t port, int interval)
    : host_(host)
    , port_(port)
    , update_interval_ms_(interval)
    , running_(false) {}

WebSocketServer::~WebSocketServer() {
    stop();
}

bool WebSocketServer::start() {
    if (running_) return true;
    running_ = true;
    ws_thread_ = std::thread(&WebSocketServer::wsLoop, this);
    std::cout << "[WS] WebSocket server starting on " << host_ << ":" << port_ << std::endl;
    return true;
}

void WebSocketServer::stop() {
    running_ = false;
    if (ws_thread_.joinable()) ws_thread_.join();
    std::cout << "[WS] WebSocket server stopped" << std::endl;
}

bool WebSocketServer::isRunning() const {
    return running_;
}

void WebSocketServer::setFrameHandler(WSFrameHandler handler) {
    frame_handler_ = std::move(handler);
}

void WebSocketServer::setUpdateGenerator(WSUpdateGenerator generator) {
    update_gen_ = std::move(generator);
}

void WebSocketServer::wsLoop() {
    while (running_) {
        if (update_gen_) {
            broadcast(update_gen_());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(update_interval_ms_));
    }
}

void WebSocketServer::broadcast(const std::string& message) {
    std::lock_guard<std::mutex> lock(client_mutex_);
    for (auto it = clients_.begin(); it != clients_.end();) {
        int nbytes = ::write(*it, message.c_str(), message.size());
        if (nbytes < 0) {
            ::close(*it);
            it = clients_.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace ecumult
