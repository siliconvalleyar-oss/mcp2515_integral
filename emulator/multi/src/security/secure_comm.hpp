#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace ecumult {

class SecureCommunication {
public:
    SecureCommunication();

    // Encrypt/decrypt diagnostic data
    std::vector<uint8_t> encrypt(const std::vector<uint8_t>& data, const std::string& key = "");
    std::vector<uint8_t> decrypt(const std::vector<uint8_t>& data, const std::string& key = "");

    // Message authentication
    std::vector<uint8_t> sign(const std::vector<uint8_t>& data);
    bool verify(const std::vector<uint8_t>& data, const std::vector<uint8_t>& signature);

    // Key management
    void setSessionKey(const std::vector<uint8_t>& key);
    std::vector<uint8_t> getSessionKey() const;
    void rotateKey();

    // ISO 14229-4 secure communication
    std::vector<uint8_t> handleSecuredMessage(const std::vector<uint8_t>& message);

private:
    std::vector<uint8_t> session_key_;
    uint32_t message_counter_;

    uint8_t xorShift(const std::vector<uint8_t>& data, const std::vector<uint8_t>& key) const;
};

} // namespace ecumult
