#pragma once

#include <string>
#include <unordered_map>

namespace ecumult {

class VINDecoder {
public:
    struct VINInfo {
        std::string vin;
        std::string wmi;
        std::string vds;
        std::string vis;
        std::string manufacturer;
        std::string make;
        std::string model;
        int year;
        std::string country;
        std::string vehicle_type;
    };

    static VINInfo decode(const std::string& vin);
    static std::string generateVIN(const std::string& manufacturer,
                                    const std::string& model, int year);
    static bool validateChecksum(const std::string& vin);
    static char calculateChecksum(const std::string& vin);
    static int getModelYear(char code);
    static std::string getCountry(const std::string& wmi);

private:
    static const std::unordered_map<char, int>& getWeightMap();
    static const std::unordered_map<char, char>& getTransliterationMap();
    static std::string getMakeFromWMI(const std::string& wmi);
};

} // namespace ecumult
