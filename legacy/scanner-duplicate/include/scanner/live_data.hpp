#ifndef SCANNER_LIVE_DATA_HPP
#define SCANNER_LIVE_DATA_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include "scanner/obd2.hpp"

namespace Scanner {

class LiveData {
public:
    struct DataChannel {
        std::string id;
        std::string name;
        std::string unit;
        float value;
        float min;
        float max;
        bool selected;
        int displayMode;  // 0=Text, 1=Gauge, 2=Wave, 3=Digital
    };

    LiveData(std::shared_ptr<OBD2> obdInterface);
    ~LiveData();

    bool initialize();
    void cleanup();

    // Data management
    bool refresh();
    std::vector<DataChannel> getSelectedChannels() const;
    void selectChannel(const std::string& id, bool selected);
    void setDisplayMode(const std::string& id, int mode);

    // Predefined lists
    std::vector<std::string> getCustomList() const;
    void loadCustomList(const std::string& filename);
    void saveCustomList(const std::string& filename) const;

private:
    std::shared_ptr<OBD2> obd_;
    bool initialized_;
    std::vector<DataChannel> channels_;
    std::unordered_map<std::string, size_t> channelIndex_;

    void setupChannels();
    void updateChannel(DataChannel& channel, const std::string& value);
};

} // namespace Scanner

#endif // SCANNER_LIVE_DATA_HPP
