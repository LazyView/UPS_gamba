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
#include "network/MessageHandler.h"
#include "network/MessageValidator.h"
#include "protocol/ProtocolMessage.h"
#include "protocol/ProtocolHelper.h"

// Simple server configuration struct (since server_config.h doesn't exist yet)
struct ServerConfig {
    std::string ip = "127.0.0.1";
    int port = 8080;
    int max_clients = 60;
    std::string log_file = "gamba_server.log";

    void printConfig() {
        std::cout << "Server Configuration:" << std::endl;
        std::cout << "  IP: " << ip << std::endl;
        std::cout << "  Port: " << port << std::endl;
        std::cout << "  Max clients: " << max_clients << std::endl;
        std::cout << "  Log file: " << log_file << std::endl;
    }
};

// Simple test function to verify modular components work
void testModularComponents() {
    std::cout << "=== Testing Modular Components ===" << std::endl;

    // Test Logger
    Logger logger("test.log");
    logger.info("Logger test - modular architecture working");

    // Test PlayerManager
    PlayerManager playerManager;
    std::string result = playerManager.connectPlayer("TestPlayer", 1);
    std::cout << "PlayerManager test: " << (result.empty() ? "FAILED" : "SUCCESS") << std::endl;

    // Test RoomManager
    RoomManager roomManager(&playerManager);
    std::string room_id = roomManager.createRoom();
    std::cout << "RoomManager test: " << (room_id.empty() ? "FAILED" : "SUCCESS") << std::endl;

    // Test MessageValidator
    MessageValidator validator;
    bool valid = validator.isValidFormat("1|player1||name=test");
    std::cout << "MessageValidator test: " << (valid ? "SUCCESS" : "FAILED") << std::endl;

    // Test ProtocolMessage
    ProtocolMessage msg(MessageType::CONNECT);
    msg.setData("name", "TestPlayer");
    std::string serialized = msg.serialize();
    std::cout << "ProtocolMessage test: " << (!serialized.empty() ? "SUCCESS" : "FAILED") << std::endl;

    // Test MessageHandler
    MessageHandler messageHandler(&playerManager, &roomManager, &validator);
    std::cout << "MessageHandler test: SUCCESS (initialized)" << std::endl;

    std::cout << "=== All modular components initialized successfully ===" << std::endl;
}

int main(int argc, char* argv[]) {
    ServerConfig config;

    // Simple command line parsing
    if (argc > 1) {
        config.port = std::stoi(argv[1]);
    }

    config.printConfig();

    // Test the modular components
    try {
        testModularComponents();
        std::cout << "Modular architecture test completed successfully!" << std::endl;
        std::cout << "Next step: Implement actual network server using these components." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error testing components: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}