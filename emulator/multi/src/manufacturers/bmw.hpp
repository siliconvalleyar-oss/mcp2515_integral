#pragma once

#include "base.hpp"

namespace ecumult {

class BMW : public BaseManufacturer {
public:
    using BaseManufacturer::BaseManufacturer;

    void handleCanFrame(const CANFrame& frame) override;
    Manufacturer getManufacturerId() const override { return Manufacturer::BMW; }
    std::string getManufacturerName() const override { return "BMW"; }

    // BMW-specific: DCAN protocol, EDIABAS/INPA
    std::vector<uint8_t> handleBMWDCAN(const std::vector<uint8_t>& data);
    std::vector<uint8_t> handleBMWJob(const std::string& job_name, const std::vector<uint8_t>& params);
    std::vector<uint8_t> handleBMWEncoding(const std::vector<uint8_t>& data);

    // Comfort/functions
    std::vector<uint8_t> handleFRM(const std::vector<uint8_t>& data);
    std::vector<uint8_t> handleCAS(const std::vector<uint8_t>& data);

private:
    void initBMWPIDs();
    std::string last_bmw_job_;
};

} // namespace ecumult
