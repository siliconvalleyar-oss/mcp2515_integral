#include "obd2_rpi/pid.hpp"
#include <algorithm>

PIDRegistry& PIDRegistry::instance() {
    static PIDRegistry reg;
    return reg;
}

void PIDRegistry::registerPID(const PIDDef& def) {
    pids_[def.pid] = def;
}

const PIDDef* PIDRegistry::find(const std::string& pid) const {
    auto it = pids_.find(pid);
    return it != pids_.end() ? &it->second : nullptr;
}

std::vector<const PIDDef*> PIDRegistry::byGroup(PIDGroup group) const {
    std::vector<const PIDDef*> result;
    for (auto& [k, v] : pids_) {
        if (v.group == group) result.push_back(&v);
    }
    std::sort(result.begin(), result.end(),
        [](const PIDDef* a, const PIDDef* b) { return a->priority < b->priority; });
    return result;
}

std::vector<const PIDDef*> PIDRegistry::all() const {
    std::vector<const PIDDef*> result;
    for (auto& [k, v] : pids_) result.push_back(&v);
    return result;
}

void PIDRegistry::loadStandard() {
    registerPID({"RPM",        "010C", "rpm",  2, [](auto b) { return (b[0]*256+b[1])/4.0; }, PIDGroup::BASIC, 0, "Engine RPM"});
    registerPID({"Speed",      "010D", "km/h", 1, [](auto b) { return (double)b[0]; }, PIDGroup::BASIC, 0, "Vehicle speed"});
    registerPID({"Coolant",    "0105", "°C",   1, [](auto b) { return (double)(b[0]-40); }, PIDGroup::BASIC, 0, "Coolant temperature"});
    registerPID({"EngineLoad", "0104", "%",    1, [](auto b) { return b[0]*100.0/255.0; }, PIDGroup::BASIC, 0, "Calculated engine load"});
    registerPID({"Throttle",   "0111", "%",    1, [](auto b) { return b[0]*100.0/255.0; }, PIDGroup::ENGINE, 2, "Throttle position"});
    registerPID({"MAP",        "010B", "kPa",  1, [](auto b) { return (double)b[0]; }, PIDGroup::ENGINE, 2, "Intake manifold pressure"});
    registerPID({"IntakeTemp", "010F", "°C",   1, [](auto b) { return (double)(b[0]-40); }, PIDGroup::ENGINE, 3, "Intake air temperature"});
    registerPID({"Timing",     "010E", "deg",  1, [](auto b) { return b[0]/2.0-64.0; }, PIDGroup::ENGINE, 3, "Timing advance"});
    registerPID({"MAF",        "0110", "g/s",  2, [](auto b) { return (b[0]*256+b[1])/100.0; }, PIDGroup::ENGINE, 1, "Mass air flow"});
    registerPID({"FuelLevel",  "012F", "%",    1, [](auto b) { return b[0]*100.0/255.0; }, PIDGroup::FUEL, 5, "Fuel level"});
    registerPID({"FuelPress",  "010A", "kPa",  1, [](auto b) { return b[0]*3.0; }, PIDGroup::FUEL, 5, "Fuel pressure"});
    registerPID({"STFT_B1",    "0106", "%",    1, [](auto b) { return (b[0]-128)*100.0/128.0; }, PIDGroup::FUEL, 1, "Short term fuel trim bank 1"});
    registerPID({"STFT_B2",    "0108", "%",    1, [](auto b) { return (b[0]-128)*100.0/128.0; }, PIDGroup::FUEL, 1, "Short term fuel trim bank 2"});
    registerPID({"LTFT_B1",    "0107", "%",    1, [](auto b) { return (b[0]-128)*100.0/128.0; }, PIDGroup::FUEL, 1, "Long term fuel trim bank 1"});
    registerPID({"LTFT_B2",    "0109", "%",    1, [](auto b) { return (b[0]-128)*100.0/128.0; }, PIDGroup::FUEL, 1, "Long term fuel trim bank 2"});
    registerPID({"BaroPress",  "0133", "kPa",  1, [](auto b) { return (double)b[0]; }, PIDGroup::ENGINE, 5, "Barometric pressure"});
    registerPID({"AmbientTemp","0146", "°C",   1, [](auto b) { return (double)(b[0]-40); }, PIDGroup::ENGINE, 5, "Ambient air temperature"});
}

void PIDRegistry::loadGM() {
    registerPID({"Odometer",   "B100", "km",   4, nullptr, PIDGroup::GM, 0, "GM odometer (UDS 0xB100)"});
    registerPID({"CatTemp",    "01B4", "°C",   2, nullptr, PIDGroup::GM, 1, "Catalyst temperature bank 1"});
    registerPID({"FuelPressGM","1180", "kPa",  2, nullptr, PIDGroup::GM, 1, "GM fuel pressure"});
    registerPID({"EngineTrq",  "01A9", "Nm",   2, nullptr, PIDGroup::GM, 1, "Engine torque"});
    registerPID({"ECUVoltage", "01A1", "V",    2, nullptr, PIDGroup::GM, 0, "ECU supply voltage"});
}
