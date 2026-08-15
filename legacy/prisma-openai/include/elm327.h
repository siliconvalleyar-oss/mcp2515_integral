#ifndef ELM327_H
#define ELM327_H

#include "mcp2515/mcp2515.h"
#include "vehicle.h"

#include <string>

class ELM327 {
public:
    ELM327(MCP2515& can, Vehicle& vehicle);

    std::string processCommand(const std::string& command);

private:
    MCP2515& can_;
    Vehicle& vehicle_;

    bool echo_;
    bool spaces_;
    bool headers_;
    bool linefeeds_;

    std::string normalize(const std::string& command);

    std::string processAT(const std::string& command);

    std::string processOBD(const std::string& command);

    std::string byteToHex(uint8_t value) const;
    std::string frameToString(
        const MCP2515::CanFrame& frame) const;

    bool sendObdResponse(
        uint8_t mode,
        uint8_t pid,
        const uint8_t* data,
        uint8_t length
    );

    std::string supportedPids0100();
    std::string supportedPids0120();

    std::string formatResponse(
        uint8_t mode,
        uint8_t pid,
        const uint8_t* data,
        uint8_t length
    );
};

#endif
