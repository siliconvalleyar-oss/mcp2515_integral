#include "access_control.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <functional>

namespace ecumult {

AccessControl::AccessControl()
    : max_attempts_(5)
    , lockout_duration_(std::chrono::seconds(60)) {
    // Default engineering access
    addUser("admin", "admin123", FULL);
    addUser("diag", "diag123", DIAGNOSTIC);
}

bool AccessControl::authenticate(const std::string& user, const std::string& password) {
    auto it = users_.find(user);
    if (it == users_.end()) return false;

    if (isLockedOut(user)) return false;

    std::string hash = hashPassword(password);
    if (it->second.password_hash == hash) {
        it->second.failed_attempts = 0;
        return true;
    }

    it->second.failed_attempts++;
    if (it->second.failed_attempts >= max_attempts_) {
        lockoutUser(user);
    }
    return false;
}

bool AccessControl::authorize(AccessLevel required_level) const {
    // Default authorization check (no user context)
    return required_level <= DIAGNOSTIC;
}

bool AccessControl::authorize(const std::string& user, AccessLevel required_level) const {
    auto it = users_.find(user);
    if (it == users_.end()) return false;
    return static_cast<int>(it->second.level) >= static_cast<int>(required_level);
}

AccessControl::AccessLevel AccessControl::getAccessLevel(const std::string& user) const {
    auto it = users_.find(user);
    if (it == users_.end()) return NONE;
    return it->second.level;
}

bool AccessControl::addUser(const std::string& user, const std::string& password, AccessLevel level) {
    if (users_.find(user) != users_.end()) return false;
    users_[user] = {hashPassword(password), level, 0, std::chrono::system_clock::now()};
    return true;
}

bool AccessControl::removeUser(const std::string& user) {
    return users_.erase(user) > 0;
}

void AccessControl::setMaxAttempts(int attempts) {
    max_attempts_ = std::max(1, attempts);
}

bool AccessControl::isLockedOut(const std::string& user) const {
    auto it = users_.find(user);
    if (it == users_.end()) return false;
    auto now = std::chrono::system_clock::now();
    return now < it->second.locked_until;
}

void AccessControl::lockoutUser(const std::string& user) {
    auto it = users_.find(user);
    if (it != users_.end()) {
        it->second.locked_until = std::chrono::system_clock::now() + lockout_duration_;
    }
}

void AccessControl::unlockUser(const std::string& user) {
    auto it = users_.find(user);
    if (it != users_.end()) {
        it->second.failed_attempts = 0;
        it->second.locked_until = std::chrono::system_clock::now();
    }
}

AccessControl::AccessLevel AccessControl::levelFromString(const std::string& s) {
    std::string lower = s;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower == "none") return NONE;
    if (lower == "read" || lower == "read_only") return READ_ONLY;
    if (lower == "diagnostic" || lower == "diag") return DIAGNOSTIC;
    if (lower == "programming" || lower == "prog") return PROGRAMMING;
    if (lower == "engineering" || lower == "eng") return ENGINEERING;
    if (lower == "full") return FULL;
    return NONE;
}

std::string AccessControl::levelToString(AccessLevel level) {
    switch (level) {
        case NONE: return "none";
        case READ_ONLY: return "read_only";
        case DIAGNOSTIC: return "diagnostic";
        case PROGRAMMING: return "programming";
        case ENGINEERING: return "engineering";
        case FULL: return "full";
        default: return "none";
    }
}

std::string AccessControl::hashPassword(const std::string& password) const {
    // Simple hash for demonstration (use SHA256 in production)
    std::hash<std::string> hasher;
    auto h = hasher(password);
    std::stringstream ss;
    ss << std::hex << h;
    return ss.str();
}

} // namespace ecumult
