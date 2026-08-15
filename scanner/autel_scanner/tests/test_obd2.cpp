#include <cassert>
#include <iostream>
#include "scanner/obd2.hpp"
#include "hardware/mcp2515.hpp"

// Mock MCP2515 for testing
class MockMCP2515 : public Hardware::MCP2515 {
public:
    MockMCP2515() : Hardware::MCP2515(0, nullptr) {}

    bool sendMessage(const Hardware::CANMessage& msg) override {
        sent_.push_back(msg);
        lastMessage_ = msg;
        return true;
    }

    bool receiveMessage(Hardware::CANMessage& msg) override {
        if (!queue_.empty()) {
            msg = queue_.front();
            queue_.erase(queue_.begin());
            return true;
        }
        if (responseReceived_) {
            msg = response_;
            responseReceived_ = false;
            return true;
        }
        return false;
    }

    void setResponse(const Hardware::CANMessage& msg) {
        response_ = msg;
        responseReceived_ = true;
    }

    void setResponses(std::vector<Hardware::CANMessage> msgs) {
        queue_ = std::move(msgs);
    }

    Hardware::CANMessage lastMessage_;
    std::vector<Hardware::CANMessage> sent_;
    std::vector<Hardware::CANMessage> queue_;
    Hardware::CANMessage response_;
    bool responseReceived_ = false;
};

void testOBDSendRequest() {
    std::cout << "Testing OBD2 send request... ";

    auto mockCan = std::make_shared<MockMCP2515>();
    Scanner::OBD2 obd(mockCan);
    obd.initialize();

    uint8_t response[8] = {0};

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
    assert(rpm == 1726.0f);  // (0x1AF8) / 4 = 6904 / 4

    std::cout << "PASSED" << std::endl;
}

void testCalculateThrottle() {
    std::cout << "Testing throttle calculation... ";

    auto mockCan = std::make_shared<MockMCP2515>();
    Scanner::OBD2 obd(mockCan);

    float throttle = obd.calculateThrottle(128);
    assert(throttle > 50.0f && throttle < 51.0f);  // ~50% (128*100/255 = 50.196)

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

void testRequestVINMultiFrame() {
    std::cout << "Testing VIN multi-frame reassembly... ";

    auto mockCan = std::make_shared<MockMCP2515>();
    Scanner::OBD2 obd(mockCan);
    obd.initialize();

    // Respuesta multi-frame ISO-TP del emulador (payload de 20 bytes):
    // 49 02 01 + VIN "9BGKL48T0HB130763"
    Hardware::CANMessage ff{};
    ff.id = 0x7E8;
    ff.dlc = 8;
    ff.data[0] = 0x10;  // First Frame
    ff.data[1] = 0x14;  // totalLen = 20
    ff.data[2] = 0x49;
    ff.data[3] = 0x02;
    ff.data[4] = 0x01;  // count
    ff.data[5] = '9';
    ff.data[6] = 'B';
    ff.data[7] = 'G';

    Hardware::CANMessage cf1{};
    cf1.id = 0x7E8;
    cf1.dlc = 8;
    cf1.data[0] = 0x21;  // CF seq 1
    cf1.data[1] = 'K';
    cf1.data[2] = 'L';
    cf1.data[3] = '4';
    cf1.data[4] = '8';
    cf1.data[5] = 'T';
    cf1.data[6] = '0';
    cf1.data[7] = 'H';

    Hardware::CANMessage cf2{};
    cf2.id = 0x7E8;
    cf2.dlc = 8;
    cf2.data[0] = 0x22;  // CF seq 2
    cf2.data[1] = 'B';
    cf2.data[2] = '1';
    cf2.data[3] = '3';
    cf2.data[4] = '0';
    cf2.data[5] = '7';
    cf2.data[6] = '6';
    cf2.data[7] = '3';

    mockCan->setResponses({ff, cf1, cf2});

    std::string vin;
    bool result = obd.requestVIN(vin);
    assert(result == true);
    assert(vin == "9BGKL48T0HB130763");

    // Debe haberse enviado el Flow Control (0x7E0, PCI 0x30)
    bool fcSent = false;
    for (const auto& m : mockCan->sent_) {
        if (m.id == 0x7E0 && m.data[0] == 0x30) {
            fcSent = true;
        }
    }
    assert(fcSent);

    std::cout << "PASSED" << std::endl;
}

void testRequestVINRejectsTruncatedSingleFrame() {
    std::cout << "Testing VIN truncated single frame rejected... ";

    auto mockCan = std::make_shared<MockMCP2515>();
    Scanner::OBD2 obd(mockCan);
    obd.initialize();

    // Single frame solo puede llevar 5 chars de VIN tras "49 02":
    // es un VIN truncado, debe rechazarse (no cumple el mínimo de 11).
    Hardware::CANMessage resp{};
    resp.id = 0x7E8;
    resp.dlc = 8;
    resp.data[0] = 0x06;
    resp.data[1] = 0x49;
    resp.data[2] = 0x02;
    resp.data[3] = '1';
    resp.data[4] = 'G';
    resp.data[5] = 'N';
    resp.data[6] = 'T';
    resp.data[7] = 'A';
    mockCan->setResponse(resp);

    std::string vin;
    bool result = obd.requestVIN(vin);
    assert(result == false);
    assert(vin.empty());

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
    testRequestVINMultiFrame();
    testRequestVINRejectsTruncatedSingleFrame();

    std::cout << "\n========================================" << std::endl;
    std::cout << "  All tests PASSED!" << std::endl;
    std::cout << "========================================\n" << std::endl;

    return 0;
}
