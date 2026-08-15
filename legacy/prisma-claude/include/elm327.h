#ifndef ELM327_H
#define ELM327_H

#include <string>
#include <vector>
#include "vehicle.h"
#include "mcp2515/mcp2515.h"

// Emula la capa de comandos de un intérprete ELM327 y las respuestas OBD2
// (modos 01, 03 y 09) de la ECU de un Chevrolet Prisma, con dos vías de acceso:
//
//   1) processCommand(): interpreta texto estilo terminal ELM327
//      (comandos AT y solicitudes de PID en hexadecimal, ej: "010C").
//      Útil para pruebas desde consola sin bus CAN físico.
//
//   2) pollCanRequests(): escucha el bus CAN real a través del MCP2515
//      y responde automáticamente a solicitudes OBD2 estándar
//      (ID 0x7DF/0x7E0 -> respuesta 0x7E8), como lo haría la ECU real.
class ELM327 {
public:
    ELM327(Vehicle *vehicle, MCP2515 *can);

    std::string processCommand(const std::string &command);
    void pollCanRequests();

    void setEcho(bool value);
    void setHeadersOn(bool value);
    void setLinefeed(bool value);

private:
    Vehicle *_vehicle;
    MCP2515 *_can;

    bool _echo;
    bool _headersOn;
    bool _linefeed;
    bool _spacesOn;
    uint8_t _protocol;

    std::string handleAT(const std::string &cmd);
    std::string handleOBD(const std::string &cmd);

    std::vector<uint8_t> buildPidResponse(uint8_t mode, uint8_t pid, bool &known);
    std::string formatResponse(const std::vector<uint8_t> &bytes, uint8_t mode, uint8_t pid);

    static std::string toUpper(const std::string &s);
    static std::string trim(const std::string &s);
};

#endif // ELM327_H
