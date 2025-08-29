//
// Created by chlad on 8/29/2025.
//
// main.cpp - Gamba Game Server Entry Point
// KIV/UPS Network Programming Project

#include "../include/server_config.h"
#include "../include/gamba_server.h"

int main(int argc, char* argv[]) {
    ServerConfig config;

    // Parse command line arguments
    config.parseCommandLine(argc, argv);

    // Try to load config file if not specified in command line
    config.loadFromFile("server.conf");

    // Print configuration
    config.printConfig();

    GambaServer server(config);

    if (!server.start()) {
        std::cerr << "Failed to start server!" << std::endl;
        return 1;
    }

    server.run();
    return 0;
}