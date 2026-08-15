#include <cassert>
#include <iostream>
#include "scanner/obd2.hpp"
#include "hardware/mcp2515.hpp"

// Mock MCP2515 for testing
class MockMCP2515 : public Hardware::MCP2515 {
public:
    MockMCP2515() : Hardware::MCP2515(0, nullptr) {}

    bool sendMessage(const Hardware::CANMessage& msg) override {
        lastMessage_ = msg;
        return true;
    }

    bool receiveMessage(Hardware::CANMessage& msg) override {
        msg = response_;
        return responseReceived_;
    }

    void setResponse(const Hardware::CANMessage& msg) {
        response_ = msg;
        responseReceived_ = true;
    }

    Hardware::CANMessage lastMessage_;
    Hardware::CANMessage response_;
    bool responseReceived_ = false;
};

void testOBDSendRequest() {
    std::cout << "Testing OBD2 send request... ";

    auto mockCan = std::make_shared<MockMCP2515>();
    Scanner::OBD2 obd(mockCan);
    obd.initialize();

    uint8_t response[8] = {0};
    size_t length = 0;

    // Simulate response
    Hardware::CANMessage resp{};
    resp.id = 0x7E8;
    resp.data[0] = 0x03;
    resp.data[1] = 0x41;
    resp.data[2] = 0x0C;
    resp.data[3] = 0x1A;
    resp.data[4] = 0xF8;
    resp.dlc = 5;
    mockCan->setResponse(resp);

    bool result = obd.requestPID(0x0C, response, 2);
    assert(result == true);
    assert(response[0] == 0x1A);
    assert(response[1] == 0xF8);

    std::cout << "PASSED" << std::endl;
}

void testCalculateRPM() {
    std::cout << "Testing RPM calculation... ";

    auto mockCan = std::make_shared<MockMCP2515>();
    Scanner::OBD2 obd(mockCan);

    float rpm = obd.calculateRPM(0x1A, 0xF8);
    assert(rpm == 2816.0f);  // (0x1AF8) / 4

    std::cout << "PASSED" << std::endl;
}

void testCalculateThrottle() {
    std::cout << "Testing throttle calculation... ";

    auto mockCan = std::make_shared<MockMCP2515>();
    Scanner::OBD2 obd(mockCan);

    float throttle = obd.calculateThrottle(128);
    assert(throttle == 50.0f);  // 50%

    float throttle2 = obd.calculateThrottle(255);
    assert(throttle2 == 100.0f);  // 100%

    std::cout << "PASSED" << std::endl;
}

void testCalculateFuelTrim() {
    std::cout << "Testing fuel trim calculation... ";

    auto mockCan = std::make_shared<MockMCP2515>();
    Scanner::OBD2 obd(mockCan);

    float trim = obd.calculateFuelTrim(128);
    assert(trim == 0.0f);  // 0%

    float trim2 = obd.calculateFuelTrim(140);
    assert(trim2 > 0);  // Positive trim

    std::cout << "PASSED" << std::endl;
}

int main() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "  AUTEL Scanner - Unit Tests" << std::endl;
    std::cout << "========================================\n" << std::endl;

    testOBDSendRequest();
    testCalculateRPM();
    testCalculateThrottle();
    testCalculateFuelTrim();

    std::cout << "\n========================================" << std::endl;
    std::cout << "  All tests PASSED!" << std::endl;
    std::cout << "========================================\n" << std::endl;

    return 0;
}
