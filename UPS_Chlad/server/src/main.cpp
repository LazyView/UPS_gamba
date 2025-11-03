// main.cpp - Gamba Game Server Entry Point (Modular Version)
// KIV/UPS Network Programming Project

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

// Include your new modular components
#include "core/Logger.h"
#include "core/PlayerManager.h"
#include "core/RoomManager.h"
#include "core/GameManager.h"
#include "core/server_config.h"
#include "network/MessageHandler.h"
#include "network/MessageValidator.h"
#include "network/NetworkManager.h"
#include "protocol/ProtocolMessage.h"
#include "protocol/ProtocolHelper.h"
#include <csignal>
#include <atomic>

// Global variable for graceful shutdown
std::atomic<bool> server_running(true);

void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ". Shutting down gracefully..." << std::endl;
    server_running.store(false);
}

int main(int argc, char* argv[]) {
    ServerConfig config;

    // Check if custom config file is specified via command line
    bool custom_config_specified = false;
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--config" || std::string(argv[i]) == "-c") {
            custom_config_specified = true;
            break;
        }
    }

    // Load default configuration file only if no custom config is specified
    if (!custom_config_specified) {
        config.loadFromFile("server.conf");
    }

    // Parse command line arguments (can override config file values or load custom config)
    config.parseCommandLine(argc, argv);

    config.printConfig();

    // Set up signal handlers for graceful shutdown
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    try {
        // Initialize all components
        Logger logger(config.log_file);
        logger.setLogLevel(LogLevel::DEBUG);
        logger.setLogToFile(config.enable_file_logging);
        PlayerManager playerManager;
        RoomManager roomManager;
        GameManager gameManager;
        MessageValidator validator;
        MessageHandler messageHandler(&playerManager, &roomManager, &validator, &logger, &gameManager);

        // Initialize NetworkManager with heartbeat monitoring
        NetworkManager networkManager(&playerManager, &roomManager, &messageHandler,
                                    &validator, &logger, &config, config.ip, config.port);

        logger.info("=== Gamba Server Starting ===");
        logger.info("Server configuration loaded with " + std::to_string(config.player_timeout_seconds) +
                   "s player timeout and " + std::to_string(config.heartbeat_check_interval) + "s heartbeat check interval");

        if (!networkManager.start()) {
            logger.error("Failed to start NetworkManager");
            return 1;
        }

        // Run server in a separate thread
        std::thread server_thread([&networkManager]() {
            networkManager.run();
        });

        logger.info("Gamba server is running. Press Ctrl+C to stop.");

        // Main loop - wait for shutdown signal
        while (server_running.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        logger.info("Shutdown signal received. Stopping server...");
        networkManager.stop();

        if (server_thread.joinable()) {
            // Use timed join to prevent hanging
            // Create a future to check if join completes within timeout
            std::atomic<bool> join_completed(false);
            std::thread join_thread([&server_thread, &join_completed]() {
                server_thread.join();
                join_completed.store(true);
            });

            // Wait up to 2 seconds for graceful shutdown
            const int timeout_ms = 2000;
            std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));

            if (join_completed.load()) {
                logger.info("Server thread joined gracefully");
                join_thread.join();
            } else {
                logger.warning("Server thread join timed out after " + std::to_string(timeout_ms) + "ms, detaching thread");
                server_thread.detach();
                join_thread.detach();
            }
        }

        logger.info("=== Gamba Server Shutdown Complete ===");
        std::cout << "Server shutdown complete." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}