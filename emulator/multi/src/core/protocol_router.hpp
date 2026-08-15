#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <functional>
#include "can_manager.hpp"

namespace ecumult {

enum class Manufacturer {
    CHEVROLET,
    FORD,
    TOYOTA,
    BMW,
    VOLKSWAGEN,
    MERCEDES,
    HONDA,
    NISSAN,
    HYUNDAI,
    UNKNOWN
};

enum class ActiveProtocol {
    OBD2,
    UDS,
    KWP2000,
    GMLAN,
    HONDA_HDS,
    NISSAN_CONSULT,
    FORD_MS_CAN,
    TOYOTA_MODE21
};

struct ManufacturerConfig {
    Manufacturer id;
    std::string name;
    ActiveProtocol protocol;
    uint32_t rx_id;
    uint32_t tx_id;
    uint32_t functional_id;
    int bitrate;
    std::string vin_prefix;
};

class BaseManufacturer;

class ProtocolRouter {
public:
    ProtocolRouter(std::shared_ptr<CANManager> can_manager);
    ~ProtocolRouter() = default;

    bool selectManufacturer(Manufacturer mfr);
    Manufacturer getCurrentManufacturer() const;
    BaseManufacturer* getCurrentImplementation() const;

    bool registerManufacturer(Manufacturer mfr, std::unique_ptr<BaseManufacturer> impl);
    void routeCANFrame(const CANFrame& frame);

    static ManufacturerConfig getConfigFor(Manufacturer mfr);
    static std::string manufacturerToString(Manufacturer mfr);
    static Manufacturer stringToManufacturer(const std::string& str);

private:
    std::shared_ptr<CANManager> can_manager_;
    Manufacturer current_manufacturer_;
    std::unordered_map<Manufacturer, std::unique_ptr<BaseManufacturer>> implementations_;
};

} // namespace ecumult
