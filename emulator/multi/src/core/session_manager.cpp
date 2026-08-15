#include "session_manager.hpp"
#include <algorithm>
#include <random>

namespace ecumult {

SessionManager::SessionManager()
    : current_session_(UDSSession::DEFAULT)
    , security_level_(SecurityLevel::LOCKED)
    , tester_present_(false)
    , session_start_(std::chrono::steady_clock::now())
    , security_unlock_time_(std::chrono::steady_clock::now())
    , session_timeout_(DEFAULT_SESSION_TIMEOUT)
    , security_timeout_(DEFAULT_SECURITY_TIMEOUT)
    , active_seed_level_(0) {}

bool SessionManager::changeSession(UDSSession session) {
    if (!isSessionAllowed(session)) return false;
    current_session_ = session;
    session_start_ = std::chrono::steady_clock::now();
    if (session == UDSSession::DEFAULT) {
        security_level_ = SecurityLevel::LOCKED;
    }
    return true;
}

UDSSession SessionManager::getCurrentSession() const {
    return current_session_;
}

bool SessionManager::isSessionAllowed(UDSSession session) const {
    switch (current_session_) {
        case UDSSession::DEFAULT:
            return session == UDSSession::DEFAULT ||
                   session == UDSSession::EXTENDED ||
                   session == UDSSession::PROGRAMMING;
        case UDSSession::EXTENDED:
            return session == UDSSession::DEFAULT ||
                   session == UDSSession::EXTENDED;
        case UDSSession::PROGRAMMING:
            return session == UDSSession::DEFAULT;
        case UDSSession::SAFETY:
            return false; // must be unlocked specifically
        default:
            return false;
    }
}

bool SessionManager::requestSeed(uint8_t level) {
    if (!seed_generator_) return false;
    active_seed_level_ = level;
    last_seed_ = seed_generator_({});
    return true;
}

bool SessionManager::sendKey(uint8_t level, const std::vector<uint8_t>& key) {
    if (level != active_seed_level_ || last_seed_.empty()) return false;
    if (!key_validator_) return false;

    if (key_validator_(last_seed_, key)) {
        security_level_ = static_cast<SecurityLevel>(level);
        security_unlock_time_ = std::chrono::steady_clock::now();
        return true;
    }
    return false;
}

bool SessionManager::isSecurityUnlocked(SecurityLevel level) const {
    return static_cast<uint8_t>(security_level_) >= static_cast<uint8_t>(level);
}

SecurityLevel SessionManager::getCurrentSecurityLevel() const {
    return security_level_;
}

void SessionManager::lock() {
    security_level_ = SecurityLevel::LOCKED;
    last_seed_.clear();
    active_seed_level_ = 0;
}

void SessionManager::setSessionTimeout(std::chrono::seconds timeout) {
    session_timeout_ = timeout;
}

void SessionManager::setSecurityTimeout(std::chrono::seconds timeout) {
    security_timeout_ = timeout;
}

bool SessionManager::isSessionExpired() const {
    if (tester_present_) return false;
    auto now = std::chrono::steady_clock::now();
    return (now - session_start_) > session_timeout_;
}

bool SessionManager::isSecurityExpired() const {
    if (tester_present_) return false;
    auto now = std::chrono::steady_clock::now();
    return (now - security_unlock_time_) > security_timeout_;
}

void SessionManager::resetTimers() {
    session_start_ = std::chrono::steady_clock::now();
    security_unlock_time_ = std::chrono::steady_clock::now();
}

void SessionManager::setTesterPresent(bool present) {
    tester_present_ = present;
}

bool SessionManager::getTesterPresent() const {
    return tester_present_;
}

void SessionManager::setTimeoutCallback(TimeoutCallback cb) {
    timeout_cb_ = std::move(cb);
}

void SessionManager::tick() {
    if (isSessionExpired() && current_session_ != UDSSession::DEFAULT) {
        changeSession(UDSSession::DEFAULT);
        if (timeout_cb_) timeout_cb_(UDSSession::DEFAULT);
    }
    if (isSecurityExpired() && security_level_ != SecurityLevel::LOCKED) {
        lock();
    }
}

void SessionManager::setSeedKeyAlgorithm(uint8_t level,
    std::function<std::vector<uint8_t>(const std::vector<uint8_t>&)> seed_gen,
    std::function<bool(const std::vector<uint8_t>&, const std::vector<uint8_t>&)> key_validator) {
    (void)level;
    seed_generator_ = std::move(seed_gen);
    key_validator_ = std::move(key_validator);
}

} // namespace ecumult
