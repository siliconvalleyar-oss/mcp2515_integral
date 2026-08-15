#include "vin_decoder.hpp"
#include <algorithm>
#include <cctype>

namespace ecumult {

VINDecoder::VINInfo VINDecoder::decode(const std::string& vin) {
    VINInfo info;
    if (vin.size() < 17) return info;

    info.vin = vin;
    info.wmi = vin.substr(0, 3);
    info.vds = vin.substr(3, 6);
    info.vis = vin.substr(9, 8);
    info.country = getCountry(info.wmi);
    info.manufacturer = getMakeFromWMI(info.wmi);
    info.make = info.manufacturer;

    if (vin.size() >= 10) {
        info.year = getModelYear(vin[9]);
    }

    return info;
}

std::string VINDecoder::generateVIN(const std::string& manufacturer,
                                     const std::string& model, int year) {
    (void)model;
    // Simple VIN generation
    std::string vin;
    if (manufacturer == "Chevrolet") vin = "1G1";
    else if (manufacturer == "Ford") vin = "1FA";
    else if (manufacturer == "Toyota") vin = "JT2";
    else if (manufacturer == "BMW") vin = "WBA";
    else if (manufacturer == "Volkswagen") vin = "WVW";
    else if (manufacturer == "Mercedes") vin = "WDB";
    else if (manufacturer == "Honda") vin = "JHM";
    else if (manufacturer == "Nissan") vin = "JN1";
    else vin = "XXX";

    // VDS (5 chars)
    vin += "ZZZ1Z";

    // Year code
    vin += "K"; // 2023 = K

    // Plant code + serial
    vin += "8W123456";

    return vin;
}

bool VINDecoder::validateChecksum(const std::string& vin) {
    if (vin.size() != 17) return false;
    char actual = vin[8];
    char expected = calculateChecksum(vin);
    return actual == expected || actual == 'X';
}

char VINDecoder::calculateChecksum(const std::string& vin) {
    if (vin.size() < 17) return '0';

    const auto& weights = getWeightMap();
    const auto& trans = getTransliterationMap();

    int sum = 0;
    for (int i = 0; i < 17; i++) {
        if (i == 8) continue; // checksum position
        char c = std::toupper(vin[i]);
        int value = (c >= '0' && c <= '9') ? (c - '0') : trans.at(c);
        int weight = weights.at(static_cast<char>('0' + (i % 10)));
        sum += value * weight;
    }

    int remainder = sum % 11;
    if (remainder == 10) return 'X';
    return '0' + remainder;
}

int VINDecoder::getModelYear(char code) {
    code = std::toupper(code);
    if (code >= 'A' && code <= 'Y') return 1980 + (code - 'A');
    if (code >= '1' && code <= '9') return 2001 + (code - '1');
    return 0;
}

std::string VINDecoder::getCountry(const std::string& wmi) {
    if (wmi.empty()) return "Unknown";
    char first = wmi[0];
    if (first >= 'A' && first <= 'H') return "USA";
    if (first == 'J') return "Japan";
    if (first >= 'K' && first <= 'L') return "Korea";
    if (first >= 'N' && first <= 'T') return "Europe";
    if (first == 'V') return "France";
    if (first == 'W') return "Germany";
    if (first == 'X') return "Netherlands";
    if (first == 'Z') return "Italy";
    return "Other";
}

std::string VINDecoder::getMakeFromWMI(const std::string& wmi) {
    if (wmi.size() < 3) return "Unknown";
    if (wmi == "1G1" || wmi == "1G2" || wmi == "1G3") return "Chevrolet";
    if (wmi == "1FA" || wmi == "1FB" || wmi == "1FC") return "Ford";
    if (wmi == "JT2" || wmi == "JT3" || wmi == "JT4") return "Toyota";
    if (wmi == "WBA" || wmi == "WBS" || wmi == "WBE") return "BMW";
    if (wmi == "WVW" || wmi == "WVG") return "Volkswagen";
    if (wmi == "WDB" || wmi == "WDD" || wmi == "WDC") return "Mercedes-Benz";
    if (wmi == "JHM" || wmi == "JHG") return "Honda";
    if (wmi == "JN1" || wmi == "JN8") return "Nissan";
    if (wmi == "KMH" || wmi == "KNA") return "Hyundai/Kia";
    return "Other";
}

const std::unordered_map<char, int>& VINDecoder::getWeightMap() {
    static const std::unordered_map<char, int> weights = {
        {'1', 8}, {'2', 7}, {'3', 6}, {'4', 5}, {'5', 4},
        {'6', 3}, {'7', 2}, {'8', 10}, {'9', 9}, {'0', 1}
    };
    return weights;
}

const std::unordered_map<char, char>& VINDecoder::getTransliterationMap() {
    static const std::unordered_map<char, char> trans = {
        {'A', 1}, {'B', 2}, {'C', 3}, {'D', 4}, {'E', 5},
        {'F', 6}, {'G', 7}, {'H', 8}, {'J', 1}, {'K', 2},
        {'L', 3}, {'M', 4}, {'N', 5}, {'P', 7}, {'R', 9},
        {'S', 2}, {'T', 3}, {'U', 4}, {'V', 5}, {'W', 6},
        {'X', 7}, {'Y', 8}, {'Z', 9}
    };
    return trans;
}

} // namespace ecumult
