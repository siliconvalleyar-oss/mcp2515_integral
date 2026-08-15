#include "protocol_router.hpp"
#include "../manufacturers/base.hpp"
#include <algorithm>
#include <cctype>

namespace ecumult {

ProtocolRouter::ProtocolRouter(std::shared_ptr<CANManager> can_manager)
    : can_manager_(std::move(can_manager))
    , current_manufacturer_(Manufacturer::UNKNOWN) {}

bool ProtocolRouter::selectManufacturer(Manufacturer mfr) {
    auto it = implementations_.find(mfr);
    if (it == implementations_.end()) return false;

    current_manufacturer_ = mfr;
    auto cfg = getConfigFor(mfr);
    it->second->onManufacturerSelected(cfg);
    return true;
}

Manufacturer ProtocolRouter::getCurrentManufacturer() const {
    return current_manufacturer_;
}

BaseManufacturer* ProtocolRouter::getCurrentImplementation() const {
    auto it = implementations_.find(current_manufacturer_);
    if (it == implementations_.end()) return nullptr;
    return it->second.get();
}

bool ProtocolRouter::registerManufacturer(Manufacturer mfr, std::unique_ptr<BaseManufacturer> impl) {
    if (implementations_.find(mfr) != implementations_.end()) return false;
    implementations_[mfr] = std::move(impl);
    return true;
}

void ProtocolRouter::routeCANFrame(const CANFrame& cf) {
    auto* impl = getCurrentImplementation();
    if (!impl) return;

    impl->handleCanFrame(cf);
}

ManufacturerConfig ProtocolRouter::getConfigFor(Manufacturer mfr) {
    switch (mfr) {
        case Manufacturer::CHEVROLET:
            return {mfr, "Chevrolet/GM", ActiveProtocol::GMLAN, 0x7E8, 0x7E0, 0x7DF, 500000, "1G1"};
        case Manufacturer::FORD:
            return {mfr, "Ford", ActiveProtocol::FORD_MS_CAN, 0x7E8, 0x7E0, 0x7DF, 500000, "1FA"};
        case Manufacturer::TOYOTA:
            return {mfr, "Toyota", ActiveProtocol::TOYOTA_MODE21, 0x7E8, 0x7E0, 0x7DF, 500000, "JT2"};
        case Manufacturer::BMW:
            return {mfr, "BMW", ActiveProtocol::UDS, 0x7E8, 0x7E0, 0x7DF, 500000, "WBA"};
        case Manufacturer::VOLKSWAGEN:
            return {mfr, "Volkswagen/Audi", ActiveProtocol::UDS, 0x7E8, 0x7E0, 0x7DF, 500000, "WVW"};
        case Manufacturer::MERCEDES:
            return {mfr, "Mercedes-Benz", ActiveProtocol::UDS, 0x7E8, 0x7E0, 0x7DF, 500000, "WDB"};
        case Manufacturer::HONDA:
            return {mfr, "Honda", ActiveProtocol::HONDA_HDS, 0x7E8, 0x7E0, 0x7DF, 500000, "JHM"};
        case Manufacturer::NISSAN:
            return {mfr, "Nissan", ActiveProtocol::NISSAN_CONSULT, 0x7E8, 0x7E0, 0x7DF, 500000, "JN1"};
        case Manufacturer::HYUNDAI:
            return {mfr, "Hyundai/Kia", ActiveProtocol::UDS, 0x7E8, 0x7E0, 0x7DF, 500000, "KMH"};
        default:
            return {mfr, "Unknown", ActiveProtocol::OBD2, 0x7E8, 0x7E0, 0x7DF, 500000, "XXX"};
    }
}

std::string ProtocolRouter::manufacturerToString(Manufacturer mfr) {
    switch (mfr) {
        case Manufacturer::CHEVROLET: return "chevrolet";
        case Manufacturer::FORD:      return "ford";
        case Manufacturer::TOYOTA:    return "toyota";
        case Manufacturer::BMW:       return "bmw";
        case Manufacturer::VOLKSWAGEN: return "volkswagen";
        case Manufacturer::MERCEDES:  return "mercedes";
        case Manufacturer::HONDA:     return "honda";
        case Manufacturer::NISSAN:    return "nissan";
        case Manufacturer::HYUNDAI:   return "hyundai";
        default: return "unknown";
    }
}

Manufacturer ProtocolRouter::stringToManufacturer(const std::string& str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "chevrolet" || lower == "gm" || lower == "chevy") return Manufacturer::CHEVROLET;
    if (lower == "ford") return Manufacturer::FORD;
    if (lower == "toyota") return Manufacturer::TOYOTA;
    if (lower == "bmw") return Manufacturer::BMW;
    if (lower == "volkswagen" || lower == "vw" || lower == "audi") return Manufacturer::VOLKSWAGEN;
    if (lower == "mercedes" || lower == "mercedes-benz" || lower == "benz") return Manufacturer::MERCEDES;
    if (lower == "honda") return Manufacturer::HONDA;
    if (lower == "nissan") return Manufacturer::NISSAN;
    if (lower == "hyundai" || lower == "kia") return Manufacturer::HYUNDAI;
    return Manufacturer::UNKNOWN;
}

} // namespace ecumult
