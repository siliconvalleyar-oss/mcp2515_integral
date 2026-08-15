#pragma once
#include "elm327.hpp"
#include <string>
#include <cstdint>

class GMCommands {
public:
    explicit GMCommands(ELM327* elm);
    ~GMCommands();

    std::string getKilometers();
    std::string getCatalystTemp();
    std::string getFuelPressure();
    std::string getEngineTorque();
    std::string getECUVoltage();
    std::string getGMHistory();
    std::string clearGMHistory();
    std::string resetAdaptations();
    void        scanPIDs(uint16_t start = 0xB000, uint16_t end = 0xB1FF);

    std::string sendCommand(const std::string& pidHex, bool useGMHeader = true);

    std::string decodeKilometers(const std::string& hexResponse);
    std::string decodeCatalystTemp(const std::string& hexResponse);
    std::string decodeFuelPressure(const std::string& hexResponse);
    std::string decodeEngineTorque(const std::string& hexResponse);
    std::string decodeECUVoltage(const std::string& hexResponse);
    std::string decodeGMHistory(const std::string& hexResponse);

private:
    ELM327* elm;
    bool setupGMHeader();
    bool restoreHeader();
    std::string processResponse(const std::string& response, const std::string& expectedPrefix = "62");
    void showError(const std::string& cmd, const std::string& response);
};
