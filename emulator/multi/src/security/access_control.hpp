#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <chrono>

namespace ecumult {

class AccessControl {
public:
    enum AccessLevel {
        NONE,
        READ_ONLY,
        DIAGNOSTIC,
        PROGRAMMING,
        ENGINEERING,
        FULL
    };

    AccessControl();

    bool authenticate(const std::string& user, const std::string& password);
    bool authorize(AccessLevel required_level) const;
    bool authorize(const std::string& user, AccessLevel required_level) const;
    AccessLevel getAccessLevel(const std::string& user) const;

    bool addUser(const std::string& user, const std::string& password, AccessLevel level);
    bool removeUser(const std::string& user);

    void setMaxAttempts(int attempts);
    bool isLockedOut(const std::string& user) const;
    void lockoutUser(const std::string& user);
    void unlockUser(const std::string& user);

    static AccessLevel levelFromString(const std::string& s);
    static std::string levelToString(AccessLevel level);

private:
    struct UserInfo {
        std::string password_hash;
        AccessLevel level;
        int failed_attempts;
        std::chrono::system_clock::time_point locked_until;
    };

    std::unordered_map<std::string, UserInfo> users_;
    int max_attempts_;
    std::chrono::seconds lockout_duration_;

    std::string hashPassword(const std::string& password) const;
};

} // namespace ecumult
