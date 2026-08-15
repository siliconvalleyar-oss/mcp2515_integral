#pragma once

#include <string>
#include <vector>

namespace ecumult {

struct SeedVehicle {
    std::string vin;
    std::string make;
    std::string model;
    int year;
};

struct SeedDTC {
    std::string code;
    std::string description;
    std::string severity;
};

struct SeedParameter {
    std::string name;
    double value;
    std::string unit;
};

class SeedData {
public:
    static std::vector<SeedVehicle> getVehicles();
    static std::vector<SeedDTC> getDefaultDTCs();
    static std::vector<SeedParameter> getDefaultParameters();
    static std::vector<std::string> getSeedSQL();
};

} // namespace ecumult
