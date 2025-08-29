//
// Created by chlad on 8/29/2025.
//
// server_config.cpp - Server Configuration Management Implementation
// KIV/UPS Network Programming Project

#include "../include/server_config.h"

bool ServerConfig::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "Config file '" << filename << "' not found, using defaults" << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;

        size_t pos = line.find('=');
        if (pos == std::string::npos) continue;

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        // Trim whitespace
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        // Parse configuration values
        if (key == "ip") ip = value;
        else if (key == "port") port = std::stoi(value);
        else if (key == "max_rooms") max_rooms = std::stoi(value);
        else if (key == "max_players_per_room") max_players_per_room = std::stoi(value);
        else if (key == "max_clients") max_clients = std::stoi(value);
        else if (key == "invalid_message_limit") invalid_message_limit = std::stoi(value);
        else if (key == "log_file") log_file = value;
        else if (key == "enable_file_logging") enable_file_logging = (value == "true");
    }

    file.close();
    std::cout << "Configuration loaded from: " << filename << std::endl;
    return true;
}

void ServerConfig::parseCommandLine(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            exit(0);
        }
        else if (arg == "--ip" && i + 1 < argc) {
            ip = argv[++i];
        }
        else if (arg == "--port" && i + 1 < argc) {
            port = std::atoi(argv[++i]);
        }
        else if (arg == "--max-rooms" && i + 1 < argc) {
            max_rooms = std::atoi(argv[++i]);
        }
        else if (arg == "--max-clients" && i + 1 < argc) {
            max_clients = std::atoi(argv[++i]);
        }
        else if (arg == "--config" && i + 1 < argc) {
            std::string config_file = argv[++i];
            loadFromFile(config_file);
        }
        else if (arg == "--no-file-log") {
            enable_file_logging = false;
        }
        else {
            std::cout << "Unknown argument: " << arg << std::endl;
            std::cout << "Use --help for usage information" << std::endl;
        }
    }
}

void ServerConfig::printUsage(const char* program_name) {
    std::cout << "Gamba Game Server - KIV/UPS Network Programming Project\n\n";
    std::cout << "Usage: " << program_name << " [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --ip <address>        Server IP address (default: " << ip << ")\n";
    std::cout << "  --port <port>         Server port (default: " << port << ")\n";
    std::cout << "  --max-rooms <num>     Maximum rooms (default: " << max_rooms << ")\n";
    std::cout << "  --max-clients <num>   Maximum clients (default: " << max_clients << ")\n";
    std::cout << "  --config <file>       Load configuration from file\n";
    std::cout << "  --no-file-log         Disable file logging\n";
    std::cout << "  --help, -h            Show this help\n\n";
    std::cout << "Example:\n";
    std::cout << "  " << program_name << " --port 9000 --max-rooms 5\n";
    std::cout << "  " << program_name << " --config server.conf\n\n";
}

void ServerConfig::printConfig() {
    std::cout << "=== SERVER CONFIGURATION ===" << std::endl;
    std::cout << "IP Address: " << ip << std::endl;
    std::cout << "Port: " << port << std::endl;
    std::cout << "Max Rooms: " << max_rooms << std::endl;
    std::cout << "Max Players/Room: " << max_players_per_room << std::endl;
    std::cout << "Max Total Clients: " << max_clients << std::endl;
    std::cout << "Invalid Message Limit: " << invalid_message_limit << std::endl;
    std::cout << "File Logging: " << (enable_file_logging ? "enabled" : "disabled") << std::endl;
    if (enable_file_logging) {
        std::cout << "Log File: " << log_file << std::endl;
    }
    std::cout << "=============================" << std::endl;
}