#pragma once

#include "base.hpp"

namespace ecumult {

class ChevroletGM : public BaseManufacturer {
public:
    using BaseManufacturer::BaseManufacturer;

    void handleCanFrame(const CANFrame& frame) override;
    Manufacturer getManufacturerId() const override { return Manufacturer::CHEVROLET; }
    std::string getManufacturerName() const override { return "Chevrolet/GM"; }

    // GMLAN specific
    std::vector<uint8_t> handleGMLANMode22(uint16_t did, const std::vector<uint8_t>& data);
    std::vector<uint8_t> handleGMLANTesterPresent();
    std::vector<uint8_t> handleGMLANReportProgrammedState();

    // GM-specific DIDs (200+)
    std::vector<uint8_t> readGMDID(uint16_t did);
    bool writeGMDID(uint16_t did, const std::vector<uint8_t>& data);

    static uint16_t dtcStringToCode(const std::string& code);
    static std::string dtcCodeToString(uint16_t code);

private:
    void initGMPIDs();
    void respondToFunctionalBroadcast();
};

} // namespace ecumult
