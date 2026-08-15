#pragma once

#include <cstdint>
#include <chrono>
#include <functional>
#include <unordered_map>

namespace ecumult {

enum class UDSSession : uint8_t {
    DEFAULT         = 0x01,
    PROGRAMMING     = 0x02,
    EXTENDED        = 0x03,
    SAFETY          = 0x04
};

enum class SecurityLevel : uint8_t {
    LOCKED          = 0x00,
    LEVEL_1         = 0x01,
    LEVEL_2         = 0x02,
    LEVEL_3         = 0x03
};

class SessionManager {
public:
    using TimeoutCallback = std::function<void(UDSSession)>;

    SessionManager();

    bool changeSession(UDSSession session);
    UDSSession getCurrentSession() const;
    bool isSessionAllowed(UDSSession session) const;

    bool requestSeed(uint8_t level);
    bool sendKey(uint8_t level, const std::vector<uint8_t>& key);
    bool isSecurityUnlocked(SecurityLevel level) const;
    SecurityLevel getCurrentSecurityLevel() const;
    void lock();

    void setSessionTimeout(std::chrono::seconds timeout);
    void setSecurityTimeout(std::chrono::seconds timeout);
    bool isSessionExpired() const;
    bool isSecurityExpired() const;

    void resetTimers();
    void setTesterPresent(bool present);
    bool getTesterPresent() const;

    void setTimeoutCallback(TimeoutCallback cb);
    void tick(); // called periodically to check timeouts

    void setSeedKeyAlgorithm(uint8_t level,
        std::function<std::vector<uint8_t>(const std::vector<uint8_t>&)> seed_gen,
        std::function<bool(const std::vector<uint8_t>&, const std::vector<uint8_t>&)> key_validator);

private:
    UDSSession current_session_;
    SecurityLevel security_level_;
    bool tester_present_;

    std::chrono::steady_clock::time_point session_start_;
    std::chrono::steady_clock::time_point security_unlock_time_;
    std::chrono::seconds session_timeout_;
    std::chrono::seconds security_timeout_;

    TimeoutCallback timeout_cb_;

    uint8_t active_seed_level_;
    std::vector<uint8_t> last_seed_;
    std::function<std::vector<uint8_t>(const std::vector<uint8_t>&)> seed_generator_;
    std::function<bool(const std::vector<uint8_t>&, const std::vector<uint8_t>&)> key_validator_;

    static constexpr std::chrono::seconds DEFAULT_SESSION_TIMEOUT{5};
    static constexpr std::chrono::seconds DEFAULT_SECURITY_TIMEOUT{10};
};

} // namespace ecumult
