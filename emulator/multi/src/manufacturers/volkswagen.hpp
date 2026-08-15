#pragma once

#include "base.hpp"

namespace ecumult {

class Volkswagen : public BaseManufacturer {
public:
    using BaseManufacturer::BaseManufacturer;

    void handleCanFrame(const CANFrame& frame) override;
    Manufacturer getManufacturerId() const override { return Manufacturer::VOLKSWAGEN; }
    std::string getManufacturerName() const override { return "Volkswagen/Audi"; }

    // VW/Audi specific
    std::vector<uint8_t> handleVWAdaptation(uint16_t channel, const std::vector<uint8_t>& data);
    std::vector<uint8_t> handleVWLongCoding(const std::vector<uint8_t>& data);
    std::vector<uint8_t> handleVWComponentProtection(const std::vector<uint8_t>& data);

private:
    void initVWPIDs();
};

} // namespace ecumult
