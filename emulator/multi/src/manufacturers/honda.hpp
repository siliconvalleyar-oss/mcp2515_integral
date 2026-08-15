#pragma once

#include "base.hpp"

namespace ecumult {

class Honda : public BaseManufacturer {
public:
    using BaseManufacturer::BaseManufacturer;

    void handleCanFrame(const CANFrame& frame) override;
    Manufacturer getManufacturerId() const override { return Manufacturer::HONDA; }
    std::string getManufacturerName() const override { return "Honda"; }

    // Honda specific: HDS protocol
    std::vector<uint8_t> handleHondaHDS(const std::vector<uint8_t>& data);
    std::vector<uint8_t> handleHondaImmobilizer(const std::vector<uint8_t>& data);
    std::vector<uint8_t> handleHondaVTEC(const std::vector<uint8_t>& data);

private:
    void initHondaPIDs();
};

} // namespace ecumult
