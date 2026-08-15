#pragma once

#include "base.hpp"

namespace ecumult {

class Nissan : public BaseManufacturer {
public:
    using BaseManufacturer::BaseManufacturer;

    void handleCanFrame(const CANFrame& frame) override;
    Manufacturer getManufacturerId() const override { return Manufacturer::NISSAN; }
    std::string getManufacturerName() const override { return "Nissan"; }

    // Nissan specific: Consult II/III
    std::vector<uint8_t> handleConsultIII(const std::vector<uint8_t>& data);
    std::vector<uint8_t> handleNissanNATS(const std::vector<uint8_t>& data);
    std::vector<uint8_t> handleNissanBCM(const std::vector<uint8_t>& data);

private:
    void initNissanPIDs();
};

} // namespace ecumult
