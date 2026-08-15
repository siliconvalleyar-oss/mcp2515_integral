#pragma once

#include "base.hpp"

namespace ecumult {

class Ford : public BaseManufacturer {
public:
    using BaseManufacturer::BaseManufacturer;

    void handleCanFrame(const CANFrame& frame) override;
    Manufacturer getManufacturerId() const override { return Manufacturer::FORD; }
    std::string getManufacturerName() const override { return "Ford"; }

    // Ford-specific: MS-CAN switching, PATS
    std::vector<uint8_t> handleFordPATS(const std::vector<uint8_t>& data);
    std::vector<uint8_t> handleFordModuleConfig();
    std::vector<uint8_t> handleFordEnhancedPIDs(uint16_t pid);

    void setMS_CAN_Enabled(bool enabled);
    void setHS_CAN_Enabled(bool enabled);

private:
    bool ms_can_enabled_ = true;
    bool hs_can_enabled_ = true;
    void initFordPIDs();
};

} // namespace ecumult
