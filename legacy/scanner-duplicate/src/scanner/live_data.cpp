#include "scanner/live_data.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <stdexcept>

namespace Scanner {

LiveData::LiveData(std::shared_ptr<OBD2> obdInterface)
    : obd_(std::move(obdInterface)), initialized_(false) {}

LiveData::~LiveData() {
    cleanup();
}

bool LiveData::initialize() {
    if (!obd_) return false;
    setupChannels();
    initialized_ = true;
    return true;
}

void LiveData::cleanup() {
    channels_.clear();
    channelIndex_.clear();
    initialized_ = false;
}

bool LiveData::refresh() {
    if (!initialized_ || !obd_) return false;

    std::unordered_map<std::string, PIDData> data;
    if (!obd_->requestLiveData(data)) {
        return false;
    }

    for (const auto& [key, pidData] : data) {
        auto it = channelIndex_.find(key);
        if (it != channelIndex_.end()) {
            channels_[it->second].value = pidData.value;
        }
    }

    return true;
}

std::vector<LiveData::DataChannel> LiveData::getSelectedChannels() const {
    std::vector<DataChannel> selected;
    for (const auto& channel : channels_) {
        if (channel.selected) {
            selected.push_back(channel);
        }
    }
    return selected;
}

void LiveData::selectChannel(const std::string& id, bool selected) {
    auto it = channelIndex_.find(id);
    if (it != channelIndex_.end()) {
        channels_[it->second].selected = selected;
    }
}

void LiveData::setDisplayMode(const std::string& id, int mode) {
    auto it = channelIndex_.find(id);
    if (it != channelIndex_.end() && mode >= 0 && mode <= 3) {
        channels_[it->second].displayMode = mode;
    }
}

std::vector<std::string> LiveData::getCustomList() const {
    std::vector<std::string> custom;
    for (const auto& channel : channels_) {
        if (channel.selected) {
            custom.push_back(channel.id);
        }
    }
    return custom;
}

void LiveData::loadCustomList(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        selectChannel(line, true);
    }
}

void LiveData::saveCustomList(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) return;

    for (const auto& channel : channels_) {
        if (channel.selected) {
            file << channel.id << std::endl;
        }
    }
}

void LiveData::setupChannels() {
    channels_ = {
        {"rpm", "RPM", "rpm", 0.0f, 0.0f, 8000.0f, true, 0},
        {"speed", "Velocidad", "km/h", 0.0f, 0.0f, 250.0f, true, 0},
        {"coolant_temp", "Temp. Refrigerante", "°C", 0.0f, -40.0f, 215.0f, true, 0},
        {"maf", "MAF", "g/s", 0.0f, 0.0f, 1000.0f, false, 0},
        {"map", "MAP", "kPa", 0.0f, 0.0f, 255.0f, false, 0},
        {"throttle", "Acelerador", "%", 0.0f, 0.0f, 100.0f, false, 0},
        {"load", "Carga Motor", "%", 0.0f, 0.0f, 100.0f, false, 0},
        {"fuel_trim_st", "STFT", "%", 0.0f, -100.0f, 100.0f, false, 0},
        {"fuel_trim_lt", "LTFT", "%", 0.0f, -100.0f, 100.0f, false, 0},
        {"fuel_pressure", "Pres. Combustible", "kPa", 0.0f, 0.0f, 1000.0f, false, 0},
        {"intake_temp", "Temp. Admision", "°C", 0.0f, -40.0f, 215.0f, false, 0},
        {"timing_adv", "Avance Encendido", "°", 0.0f, -64.0f, 64.0f, false, 0},
        {"o2_sensor", "Sonda O2", "V", 0.0f, 0.0f, 5.0f, false, 0},
        {"fuel_status", "Estado Combustible", "", 0.0f, 0.0f, 0.0f, false, 0},
        {"boost", "Presion Turbo", "kPa", 0.0f, -100.0f, 300.0f, false, 0}
    };

    for (size_t i = 0; i < channels_.size(); ++i) {
        channelIndex_[channels_[i].id] = i;
    }
}

void LiveData::updateChannel(DataChannel& channel, const std::string& value) {
    try {
        channel.value = std::stof(value);
    } catch (...) {
        channel.value = 0.0f;
    }
}

} // namespace Scanner
