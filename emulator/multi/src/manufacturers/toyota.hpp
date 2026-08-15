#pragma once

#include "base.hpp"

namespace ecumult {

class Toyota : public BaseManufacturer {
public:
    using BaseManufacturer::BaseManufacturer;

    void handleCanFrame(const CANFrame& frame) override;
    Manufacturer getManufacturerId() const override { return Manufacturer::TOYOTA; }
    std::string getManufacturerName() const override { return "Toyota"; }

    // Toyota specific modes
    std::vector<uint8_t> handleMode21(uint8_t pid); // Toyota-specific Mode 21
    std::vector<uint8_t> handleMode22(uint8_t pid); // Toyota-specific Mode 22 (data list)

    std::vector<uint8_t> handleSmartKey(const std::vector<uint8_t>& data);
    std::vector<uint8_t> handleImmobilizer(const std::vector<uint8_t>& data);

private:
    void initToyotaPIDs();
    uint8_t toyota_checksum(const std::vector<uint8_t>& data);
};

} // namespace ecumult
