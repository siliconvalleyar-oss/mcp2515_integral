#ifndef SCANNER_DTC_HPP
#define SCANNER_DTC_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace Scanner {

struct DTC {
    std::string code;
    std::string status;  // Confirmed, Pending, Permanent
    std::string description;
    std::string severity;  // Critical, Major, Minor
    bool freezeFrameAvailable;

    std::unordered_map<std::string, std::string> freezeFrameData;
};

class DTCManager {
public:
    DTCManager();
    ~DTCManager();

    bool loadDatabase(const std::string& filename);
    const DTC* lookup(const std::string& code) const;
    std::vector<DTC> getAll() const { return dtcs_; }
    void clear();

    std::string getStatusString(uint8_t statusByte);
    std::string getSeverity(const std::string& code);

private:
    std::vector<DTC> dtcs_;
    std::unordered_map<std::string, size_t> dtcIndex_;
};

} // namespace Scanner

#endif // SCANNER_DTC_HPP
