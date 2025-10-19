// server_config.h - Server Configuration Management
// KIV/UPS Network Programming Project

#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#include <string>
#include <iostream>
#include <fstream>

struct ServerConfig {
    std::string ip = "127.0.0.1";
    int port = 8080;
    int max_rooms = 10;
    int max_players_per_room = 6;
    int max_clients = 60;
    int invalid_message_limit = 3;
    std::string log_file = "logs/gamba_server.log";
    bool enable_file_logging = true;

    // Heartbeat timeout settings
    int player_timeout_seconds = 60;        // How long before player is considered disconnected
    int heartbeat_check_interval = 10;      // How often to check for timeouts (in seconds)

    bool loadFromFile(const std::string& filename);
    void parseCommandLine(int argc, char* argv[]);
    void printUsage(const char* program_name);
    void printConfig();
};

#endif // SERVER_CONFIG_H