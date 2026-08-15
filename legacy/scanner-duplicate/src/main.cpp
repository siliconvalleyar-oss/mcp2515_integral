#include "scanner/scanner.hpp"
#include <csignal>
#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <memory>
#include <thread>

std::unique_ptr<Scanner::AutelScanner> g_scanner;

void signalHandler(int signal) {
    if (g_scanner) {
        g_scanner->shutdown();
    }
    exit(signal);
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    // Register signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGSEGV, signalHandler);

    // Check for root privileges
    if (geteuid() != 0) {
        fprintf(stderr, "Error: Este programa debe ejecutarse como root (sudo)\n");
        return 1;
    }

    // Create and initialize scanner
    g_scanner = std::make_unique<Scanner::AutelScanner>();

    if (!g_scanner->initialize()) {
        fprintf(stderr, "Error: No se pudo inicializar el scanner\n");
        return 1;
    }

    // Run main loop
    g_scanner->run();

    // Cleanup
    g_scanner->shutdown();
    g_scanner.reset();

    return 0;
}
