#include "metrics_exporter.hpp"
#include <sstream>
#include <algorithm>

namespace ecumult {

MetricsExporter::MetricsExporter() {
    gauge("ecu_frames_rx_total", 0);
    gauge("ecu_frames_tx_total", 0);
    gauge("ecu_dtc_count", 0);
    gauge("ecu_uptime_seconds", 0);
    gauge("ecu_engine_rpm", 0);
    gauge("ecu_vehicle_speed", 0);
    gauge("ecu_battery_voltage", 12.6);
}

void MetricsExporter::increment(const std::string& name, double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& m = metrics_[name];
    m.value += value;
    m.updated = std::chrono::system_clock::now();
}

void MetricsExporter::gauge(const std::string& name, double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& m = metrics_[name];
    m.value = value;
    m.updated = std::chrono::system_clock::now();
}

void MetricsExporter::observe(const std::string& name, double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& m = metrics_[name];
    m.value = (m.value + value) / 2.0; // moving average
    m.updated = std::chrono::system_clock::now();
}

void MetricsExporter::set(const std::string& name, const std::string& labels, double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& m = metrics_[name];
    m.value = value;
    m.labels = labels;
    m.updated = std::chrono::system_clock::now();
}

double MetricsExporter::get(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = metrics_.find(name);
    if (it != metrics_.end()) return it->second.value;
    return 0;
}

std::string MetricsExporter::getPrometheusText() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::stringstream ss;
    for (const auto& [name, metric] : metrics_) {
        std::string sanitized = sanitizeName(name);
        ss << "# HELP " << sanitized << " " << name << std::endl;
        ss << "# TYPE " << sanitized << " gauge" << std::endl;
        ss << sanitized;
        if (!metric.labels.empty()) {
            ss << "{" << metric.labels << "}";
        }
        ss << " " << metric.value << std::endl;
    }
    return ss.str();
}

void MetricsExporter::reset(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    metrics_.erase(name);
}

void MetricsExporter::resetAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    metrics_.clear();
}

void MetricsExporter::recordCANFrame(bool tx) {
    increment(tx ? "ecu_frames_tx_total" : "ecu_frames_rx_total");
}

void MetricsExporter::recordDiagnosticRequest(uint8_t service) {
    increment("ecu_diag_requests_total");
    std::stringstream ss;
    ss << "service=\"0x" << std::hex << (int)service << "\"";
    increment("ecu_diag_requests_by_service", 1);
}

void MetricsExporter::recordDTCSet() {
    increment("ecu_dtc_set_total");
}

void MetricsExporter::recordSessionChange(const std::string& session) {
    set("ecu_active_session", "session=\"" + session + "\"", 1);
}

std::string MetricsExporter::sanitizeName(const std::string& name) const {
    std::string s = name;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    for (auto& c : s) {
        if (!std::isalnum(c)) c = '_';
    }
    return s;
}

} // namespace ecumult
