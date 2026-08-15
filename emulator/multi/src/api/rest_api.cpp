#include "rest_api.hpp"
#include "../core/protocol_router.hpp"
#include "../database/db_manager.hpp"
#include "../manufacturers/base.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace ecumult {

RESTAPI::RESTAPI(std::shared_ptr<ProtocolRouter> router,
                 std::shared_ptr<DBManager> db,
                 const std::string& host, uint16_t port)
    : router_(std::move(router))
    , db_(std::move(db))
    , host_(host)
    , port_(port)
    , running_(false) {}

RESTAPI::~RESTAPI() {
    stop();
}

bool RESTAPI::start() {
    if (running_) return true;
    running_ = true;
    server_thread_ = std::thread(&RESTAPI::apiLoop, this);
    std::cout << "[API] REST server starting on " << host_ << ":" << port_ << std::endl;
    return true;
}

void RESTAPI::stop() {
    running_ = false;
    if (server_thread_.joinable()) server_thread_.join();
    std::cout << "[API] REST server stopped" << std::endl;
}

bool RESTAPI::isRunning() const {
    return running_;
}

void RESTAPI::setRouteHandler(RouteHandler handler) {
    custom_handler_ = std::move(handler);
}

static std::string buildResponse(int status, const std::string& body,
                                  const std::string& content_type = "application/json") {
    std::string status_text = (status == 200) ? "OK" :
                              (status == 201) ? "Created" :
                              (status == 400) ? "Bad Request" :
                              (status == 404) ? "Not Found" :
                              (status == 405) ? "Method Not Allowed" :
                              (status == 500) ? "Internal Server Error" : "Unknown";
    std::string resp =
        "HTTP/1.1 " + std::to_string(status) + " " + status_text + "\r\n"
        "Content-Type: " + content_type + "\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "Connection: close\r\n"
        "\r\n" + body;
    return resp;
}

void RESTAPI::apiLoop() {
    int server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "[API] Socket creation failed" << std::endl;
        return;
    }

    int opt = 1;
    ::setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (::bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[API] Bind failed on port " << port_ << std::endl;
        ::close(server_fd);
        return;
    }

    ::listen(server_fd, 10);
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    ::setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    std::cout << "[API] HTTP server listening on " << port_ << std::endl;

    while (running_) {
        struct sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = ::accept(server_fd, (struct sockaddr*)&client_addr, &client_len);

        if (client_fd < 0) {
            if (errno == EINTR || errno == EAGAIN) continue;
            break;
        }

        // Read request
        char buf[4096];
        ssize_t n = ::read(client_fd, buf, sizeof(buf) - 1);
        if (n <= 0) {
            ::close(client_fd);
            continue;
        }
        buf[n] = '\0';

        std::string request(buf);
        std::istringstream stream(request);

        // Parse request line: METHOD /path HTTP/1.1
        std::string method, path, http_version;
        stream >> method >> path >> http_version;

        // Parse headers and body
        std::string line;
        std::unordered_map<std::string, std::string> headers;
        int content_length = 0;
        while (std::getline(stream, line) && line != "\r") {
            if (line.back() == '\r') line.pop_back();
            auto colon = line.find(':');
            if (colon != std::string::npos) {
                std::string key = line.substr(0, colon);
                std::string val = line.substr(colon + 2);
                headers[key] = val;
                if (key == "Content-Length") {
                    content_length = std::stoi(val);
                }
            }
        }

        std::string body;
        if (content_length > 0) {
            body.resize(content_length);
            stream.read(&body[0], content_length);
        }

        // Handle CORS preflight
        if (method == "OPTIONS") {
            std::string resp = buildResponse(200, "");
            ::write(client_fd, resp.c_str(), resp.size());
            ::close(client_fd);
            continue;
        }

        std::string response_body;
        int status = 200;

        if (custom_handler_) {
            std::unordered_map<std::string, std::string> params;
            // Extract query params from path
            auto qmark = path.find('?');
            std::string clean_path = path;
            if (qmark != std::string::npos) {
                clean_path = path.substr(0, qmark);
                std::string qs = path.substr(qmark + 1);
                std::istringstream qss(qs);
                std::string pair;
                while (std::getline(qss, pair, '&')) {
                    auto eq = pair.find('=');
                    if (eq != std::string::npos) {
                        params[pair.substr(0, eq)] = pair.substr(eq + 1);
                    }
                }
            }
            response_body = custom_handler_(clean_path, method, body, params);
        } else {
            response_body = R"({"status":"ok","service":"ECU Multi-Emulator","version":"1.0.0"})";
        }

        if (response_body.empty()) {
            response_body = R"({"error":"Not Found"})";
            status = 404;
        }

        std::string http_response = buildResponse(status, response_body);
        ::write(client_fd, http_response.c_str(), http_response.size());
        ::close(client_fd);
    }

    ::close(server_fd);
}

} // namespace ecumult
