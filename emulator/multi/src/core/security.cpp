#include "security.hpp"
#include <random>
#include <cstring>
#include <algorithm>

namespace ecumult {

SecurityManager::SecurityManager() {
    seed_sizes_[1] = 4;
    seed_sizes_[2] = 4;
    seed_sizes_[3] = 8;
}

void SecurityManager::registerAlgorithm(const std::string& manufacturer,
                                         uint8_t level,
                                         SeedKeyAlgorithm algo) {
    algorithms_[{manufacturer, level}] = std::move(algo);
}

std::vector<uint8_t> SecurityManager::generateSeed(const std::string& manufacturer,
                                                     uint8_t level) {
    std::vector<uint8_t> seed(getSeedSize(level));
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 255);
    for (auto& b : seed) b = static_cast<uint8_t>(dist(rng));

    auto it = algorithms_.find({manufacturer, level});
    if (it != algorithms_.end()) {
        return it->second(seed, {}, true);
    }
    return seed;
}

bool SecurityManager::validateKey(const std::string& manufacturer,
                                   uint8_t level,
                                   const std::vector<uint8_t>& seed,
                                   const std::vector<uint8_t>& key) {
    auto it = algorithms_.find({manufacturer, level});
    if (it == algorithms_.end()) {
        // Default: simple XOR validation
        if (seed.size() != key.size()) return false;
        std::vector<uint8_t> expected(seed.size());
        for (size_t i = 0; i < seed.size(); i++) {
            expected[i] = seed[i] ^ 0xFF;
        }
        return key == expected;
    }
    auto result = it->second(seed, key, false);
    return !result.empty() && result[0] == 0x01; // convention: returns {1} for success
}

void SecurityManager::setSeedSize(uint8_t level, size_t size) {
    seed_sizes_[level] = size;
}

size_t SecurityManager::getSeedSize(uint8_t level) const {
    auto it = seed_sizes_.find(level);
    if (it != seed_sizes_.end()) return it->second;
    return 4;
}

// GM 28-bit seed/key algorithm
SecurityManager::SeedKeyAlgorithm SecurityManager::gmAlgorithm28bit() {
    return [](const std::vector<uint8_t>& seed, const std::vector<uint8_t>& key, bool gen) -> std::vector<uint8_t> {
        std::vector<uint8_t> result;
        if (gen) {
            return seed; // return raw seed
        }
        // Validate: key[3:0] should be ~(seed[3:0] + 0x55) & 0xFF... (simplified)
        if (key.size() < 4 || seed.size() < 4) return {};
        bool ok = true;
        for (int i = 0; i < 4; i++) {
            uint8_t expected = static_cast<uint8_t>(~((seed[i] + 0x55 + i) & 0xFF));
            if (key[i] != expected) ok = false;
        }
        return ok ? std::vector<uint8_t>{1} : std::vector<uint8_t>{0};
    };
}

SecurityManager::SeedKeyAlgorithm SecurityManager::gmAlgorithm40bit() {
    return [](const std::vector<uint8_t>& seed, const std::vector<uint8_t>& key, bool gen) -> std::vector<uint8_t> {
        if (gen) return seed;
        if (key.size() < 5 || seed.size() < 5) return {};
        bool ok = true;
        for (int i = 0; i < 5; i++) {
            uint8_t expected = static_cast<uint8_t>((seed[i] * 0x1B + 0x4A) & 0xFF);
            if (key[i] != expected) ok = false;
        }
        return ok ? std::vector<uint8_t>{1} : std::vector<uint8_t>{0};
    };
}

SecurityManager::SeedKeyAlgorithm SecurityManager::fordAlgorithm() {
    return [](const std::vector<uint8_t>& seed, const std::vector<uint8_t>& key, bool gen) -> std::vector<uint8_t> {
        if (gen) return seed;
        if (key.size() < 4 || seed.size() < 4) return {};
        bool ok = true;
        for (int i = 0; i < 4; i++) {
            uint8_t expected = static_cast<uint8_t>((seed[i] ^ 0x5A) + i);
            if (key[i] != expected) ok = false;
        }
        return ok ? std::vector<uint8_t>{1} : std::vector<uint8_t>{0};
    };
}

SecurityManager::SeedKeyAlgorithm SecurityManager::toyotaAlgorithm() {
    return [](const std::vector<uint8_t>& seed, const std::vector<uint8_t>& key, bool gen) -> std::vector<uint8_t> {
        if (gen) return seed;
        if (key.size() < 4 || seed.size() < 4) return {};
        bool ok = true;
        for (int i = 0; i < 4; i++) {
            uint8_t expected = static_cast<uint8_t>((seed[i] + 0xAA) ^ 0x55);
            if (key[i] != expected) ok = false;
        }
        return ok ? std::vector<uint8_t>{1} : std::vector<uint8_t>{0};
    };
}

SecurityManager::SeedKeyAlgorithm SecurityManager::bmwAlgorithm() {
    return [](const std::vector<uint8_t>& seed, const std::vector<uint8_t>& key, bool gen) -> std::vector<uint8_t> {
        if (gen) return seed;
        if (key.size() < 4 || seed.size() < 4) return {};
        // BMW rolling XOR pattern
        bool ok = true;
        uint8_t prev = 0;
        for (int i = 0; i < 4; i++) {
            uint8_t expected = static_cast<uint8_t>(seed[i] ^ 0x87 ^ prev);
            if (key[i] != expected) ok = false;
            prev = seed[i];
        }
        return ok ? std::vector<uint8_t>{1} : std::vector<uint8_t>{0};
    };
}

SecurityManager::SeedKeyAlgorithm SecurityManager::vwAlgorithm() {
    return [](const std::vector<uint8_t>& seed, const std::vector<uint8_t>& key, bool gen) -> std::vector<uint8_t> {
        if (gen) return seed;
        if (key.size() < 4 || seed.size() < 4) return {};
        bool ok = true;
        for (int i = 0; i < 4; i++) {
            uint8_t expected = static_cast<uint8_t>((seed[i] << 2) | (seed[(i+1)%4] >> 6));
            expected ^= 0x3C;
            if (key[i] != expected) ok = false;
        }
        return ok ? std::vector<uint8_t>{1} : std::vector<uint8_t>{0};
    };
}

SecurityManager::SeedKeyAlgorithm SecurityManager::mercedesAlgorithm() {
    return [](const std::vector<uint8_t>& seed, const std::vector<uint8_t>& key, bool gen) -> std::vector<uint8_t> {
        if (gen) return seed;
        if (key.size() < 8 || seed.size() < 8) return {};
        bool ok = true;
        for (int i = 0; i < 8; i++) {
            uint8_t expected = static_cast<uint8_t>((seed[i] * 0x1D + 0x3B) & 0xFF);
            if (key[i] != expected) ok = false;
        }
        return ok ? std::vector<uint8_t>{1} : std::vector<uint8_t>{0};
    };
}

SecurityManager::SeedKeyAlgorithm SecurityManager::hondaAlgorithm() {
    return [](const std::vector<uint8_t>& seed, const std::vector<uint8_t>& key, bool gen) -> std::vector<uint8_t> {
        if (gen) return seed;
        if (key.size() < 4 || seed.size() < 4) return {};
        bool ok = true;
        for (int i = 0; i < 4; i++) {
            uint8_t expected = seed[i] ^ 0x5C;
            if (key[i] != expected) ok = false;
        }
        return ok ? std::vector<uint8_t>{1} : std::vector<uint8_t>{0};
    };
}

SecurityManager::SeedKeyAlgorithm SecurityManager::nissanAlgorithm() {
    return [](const std::vector<uint8_t>& seed, const std::vector<uint8_t>& key, bool gen) -> std::vector<uint8_t> {
        if (gen) return seed;
        if (key.size() < 4 || seed.size() < 4) return {};
        bool ok = true;
        uint32_t seed32 = (seed[0] << 24) | (seed[1] << 16) | (seed[2] << 8) | seed[3];
        uint32_t key32 = (key[0] << 24) | (key[1] << 16) | (key[2] << 8) | key[3];
        uint32_t expected = ~seed32 + 0xA5A5A5A5;
        if (key32 != expected) ok = false;
        return ok ? std::vector<uint8_t>{1} : std::vector<uint8_t>{0};
    };
}

} // namespace ecumult
