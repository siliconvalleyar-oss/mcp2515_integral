#pragma once

#include "base.hpp"

namespace ecumult {

class Mercedes : public BaseManufacturer {
public:
    using BaseManufacturer::BaseManufacturer;

    void handleCanFrame(const CANFrame& frame) override;
    Manufacturer getManufacturerId() const override { return Manufacturer::MERCEDES; }
    std::string getManufacturerName() const override { return "Mercedes-Benz"; }

    // Mercedes specific: Star Diagnostics, DAS
    std::vector<uint8_t> handleMercedesDAS(const std::vector<uint8_t>& data);
    std::vector<uint8_t> handleMercedesSBC(const std::vector<uint8_t>& data);
    std::vector<uint8_t> handleMercedesSAM(const std::vector<uint8_t>& data);

private:
    void initMercedesPIDs();
};

} // namespace ecumult
