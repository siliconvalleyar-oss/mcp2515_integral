#pragma once
#include <string>
#include <vector>
#include <functional>
#include <map>

enum class PIDGroup {
    BASIC,
    FUEL,
    O2,
    DIAGNOSTIC,
    ENGINE,
    GM
};

struct PIDDef {
    std::string name;
    std::string pid;
    std::string unit;
    int numBytes;
    std::function<double(const std::vector<int>&)> formula;
    PIDGroup group;
    int priority;  // lower = more frequent
    std::string description;
};

class PIDRegistry {
public:
    static PIDRegistry& instance();

    void registerPID(const PIDDef& def);
    const PIDDef* find(const std::string& pid) const;
    std::vector<const PIDDef*> byGroup(PIDGroup group) const;
    std::vector<const PIDDef*> all() const;

    void loadStandard();
    void loadGM();

private:
    PIDRegistry() = default;
    std::map<std::string, PIDDef> pids_;
};
