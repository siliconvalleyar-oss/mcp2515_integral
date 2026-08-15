#include "scanner/scanner.hpp"
#include "scanner/event_log.hpp"
#include <csignal>
#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <memory>
#include <thread>

std::unique_ptr<Scanner::AutelScanner> g_scanner;

void signalHandler(int signal) {
    Scanner::EventLog::instance().warn("Senal " + std::to_string(signal) + " recibida, apagando");
    if (g_scanner) {
        g_scanner->shutdown();
    }
    exit(signal);
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    auto& log = Scanner::EventLog::instance();

    // Register signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGSEGV, signalHandler);

    // Check for root privileges
    if (geteuid() != 0) {
        log.error("Debe ejecutarse como root (sudo)");
        fprintf(stderr, "Error: Este programa debe ejecutarse como root (sudo)\n");
        return 1;
    }

    log.info(std::string("Iniciando scanner_autel v") + SCANNER_VERSION
             + " (log en " + log.filename() + ")");

    // Create and initialize scanner
    g_scanner = std::make_unique<Scanner::AutelScanner>();

    if (!g_scanner->initialize()) {
        log.error("No se pudo inicializar el scanner");
        fprintf(stderr, "Error: No se pudo inicializar el scanner\n");
        return 1;
    }

    // Run main loop
    g_scanner->run(SCANNER_VERSION);

    // Cleanup
    g_scanner->shutdown();
    g_scanner.reset();

    log.info("Scanner finalizado");
    return 0;
}
