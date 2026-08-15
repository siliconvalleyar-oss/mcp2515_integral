#include "secure_comm.hpp"
#include <algorithm>
#include <random>

namespace ecumult {

SecureCommunication::SecureCommunication()
    : message_counter_(0) {
    session_key_ = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
}

std::vector<uint8_t> SecureCommunication::encrypt(const std::vector<uint8_t>& data, const std::string& key) {
    (void)key;
    // Simple XOR cipher for demonstration
    std::vector<uint8_t> result = data;
    for (size_t i = 0; i < result.size(); i++) {
        result[i] ^= session_key_[i % session_key_.size()];
    }
    return result;
}

std::vector<uint8_t> SecureCommunication::decrypt(const std::vector<uint8_t>& data, const std::string& key) {
    (void)key;
    // XOR is symmetric
    return encrypt(data);
}

std::vector<uint8_t> SecureCommunication::sign(const std::vector<uint8_t>& data) {
    // Simple checksum-based signature
    uint8_t sig = 0;
    for (auto b : data) sig ^= b;
    for (auto k : session_key_) sig ^= k;
    return {sig};
}

bool SecureCommunication::verify(const std::vector<uint8_t>& data, const std::vector<uint8_t>& signature) {
    auto expected = sign(data);
    return expected == signature;
}

void SecureCommunication::setSessionKey(const std::vector<uint8_t>& key) {
    session_key_ = key;
}

std::vector<uint8_t> SecureCommunication::getSessionKey() const {
    return session_key_;
}

void SecureCommunication::rotateKey() {
    std::mt19937 rng(std::random_device{}());
    for (auto& k : session_key_) {
        k = static_cast<uint8_t>(rng() & 0xFF);
    }
}

std::vector<uint8_t> SecureCommunication::handleSecuredMessage(const std::vector<uint8_t>& message) {
    (void)message;
    return {0x50, 0x01, 0x00}; // Secure comm OK
}

uint8_t SecureCommunication::xorShift(const std::vector<uint8_t>& data, const std::vector<uint8_t>& key) const {
    uint8_t result = 0;
    for (size_t i = 0; i < data.size(); i++) {
        result ^= data[i] ^ key[i % key.size()];
    }
    return result;
}

} // namespace ecumult
