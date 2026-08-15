#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>

namespace ecumult {

class MetricsExporter {
public:
    MetricsExporter();

    void increment(const std::string& name, double value = 1.0);
    void gauge(const std::string& name, double value);
    void observe(const std::string& name, double value);
    void set(const std::string& name, const std::string& labels, double value);

    double get(const std::string& name) const;
    std::string getPrometheusText() const;
    void reset(const std::string& name);
    void resetAll();

    // Built-in metrics
    void recordCANFrame(bool tx);
    void recordDiagnosticRequest(uint8_t service);
    void recordDTCSet();
    void recordSessionChange(const std::string& session);

private:
    struct Metric {
        double value;
        std::string labels;
        std::chrono::system_clock::time_point updated;
    };

    std::unordered_map<std::string, Metric> metrics_;
    mutable std::mutex mutex_;

    std::string sanitizeName(const std::string& name) const;
};

} // namespace ecumult
