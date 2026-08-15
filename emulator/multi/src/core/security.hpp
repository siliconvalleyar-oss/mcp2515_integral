#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <functional>
#include <string>

namespace ecumult {

class SecurityManager {
public:
    SecurityManager();

    using SeedKeyAlgorithm = std::function<std::vector<uint8_t>(
        const std::vector<uint8_t>& seed, const std::vector<uint8_t>& key, bool generate_key)>;

    void registerAlgorithm(const std::string& manufacturer,
                           uint8_t level,
                           SeedKeyAlgorithm algo);

    std::vector<uint8_t> generateSeed(const std::string& manufacturer,
                                       uint8_t level);
    bool validateKey(const std::string& manufacturer,
                     uint8_t level,
                     const std::vector<uint8_t>& seed,
                     const std::vector<uint8_t>& key);

    void setSeedSize(uint8_t level, size_t size);
    size_t getSeedSize(uint8_t level) const;

    // GM specific algorithms
    static SeedKeyAlgorithm gmAlgorithm28bit();
    static SeedKeyAlgorithm gmAlgorithm40bit();

    // Ford specific
    static SeedKeyAlgorithm fordAlgorithm();

    // Toyota specific
    static SeedKeyAlgorithm toyotaAlgorithm();

    // BMW specific
    static SeedKeyAlgorithm bmwAlgorithm();

    // VW specific
    static SeedKeyAlgorithm vwAlgorithm();

    // Mercedes specific
    static SeedKeyAlgorithm mercedesAlgorithm();

    // Honda specific
    static SeedKeyAlgorithm hondaAlgorithm();

    // Nissan specific
    static SeedKeyAlgorithm nissanAlgorithm();

private:
    struct AlgorithmKey {
        std::string manufacturer;
        uint8_t level;
        bool operator==(const AlgorithmKey& o) const {
            return manufacturer == o.manufacturer && level == o.level;
        }
    };

    struct AlgorithmKeyHash {
        size_t operator()(const AlgorithmKey& k) const {
            return std::hash<std::string>()(k.manufacturer) ^ (k.level << 8);
        }
    };

    std::unordered_map<AlgorithmKey, SeedKeyAlgorithm, AlgorithmKeyHash> algorithms_;
    std::unordered_map<uint8_t, size_t> seed_sizes_;
};

} // namespace ecumult
