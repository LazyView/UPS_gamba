// ServerConfig.cpp - Server Configuration Management Implementation
// KIV/UPS Network Programming Project

#include "core/server_config.h"
#include <sstream>
#include <algorithm>
#include <cctype>

bool ServerConfig::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open configuration file: " << filename << std::endl;
        std::cerr << "Using default configuration values." << std::endl;
        return false;
    }

    std::string line;
    int line_number = 0;
    bool has_errors = false;

    while (std::getline(file, line)) {
        line_number++;

        // Remove leading and trailing whitespace
        line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
        line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), line.end());

        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Find the '=' separator
        size_t pos = line.find('=');
        if (pos == std::string::npos) {
            std::cerr << "Warning: Invalid format at line " << line_number
                      << " in " << filename << ": " << line << std::endl;
            has_errors = true;
            continue;
        }

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        // Remove whitespace from key and value
        key.erase(key.begin(), std::find_if(key.begin(), key.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
        key.erase(std::find_if(key.rbegin(), key.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), key.end());

        value.erase(value.begin(), std::find_if(value.begin(), value.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
        value.erase(std::find_if(value.rbegin(), value.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), value.end());

        // Parse configuration values
        try {
            if (key == "ip") {
                ip = value;
            } else if (key == "port") {
                port = std::stoi(value);
                if (port < 1 || port > 65535) {
                    std::cerr << "Warning: Invalid port number " << port
                              << " at line " << line_number << ". Using default: 8080" << std::endl;
                    port = 8080;
                    has_errors = true;
                }
            } else if (key == "max_rooms") {
                max_rooms = std::stoi(value);
                if (max_rooms < 1) {
                    std::cerr << "Warning: Invalid max_rooms " << max_rooms
                              << " at line " << line_number << ". Using default: 15" << std::endl;
                    max_rooms = 15;
                    has_errors = true;
                }
            } else if (key == "log_file") {
                log_file = value;
            } else if (key == "enable_file_logging") {
                std::string lower_value = value;
                std::transform(lower_value.begin(), lower_value.end(), lower_value.begin(), ::tolower);
                if (lower_value == "true" || lower_value == "1" || lower_value == "yes") {
                    enable_file_logging = true;
                } else if (lower_value == "false" || lower_value == "0" || lower_value == "no") {
                    enable_file_logging = false;
                } else {
                    std::cerr << "Warning: Invalid enable_file_logging value '" << value
                              << "' at line " << line_number << ". Using default: true" << std::endl;
                    enable_file_logging = true;
                    has_errors = true;
                }
            } else if (key == "player_timeout_seconds") {
                player_timeout_seconds = std::stoi(value);
                if (player_timeout_seconds < 5) {
                    std::cerr << "Warning: Invalid player_timeout_seconds " << player_timeout_seconds
                              << " at line " << line_number << ". Using default: 60" << std::endl;
                    player_timeout_seconds = 60;
                    has_errors = true;
                }
            } else if (key == "heartbeat_check_interval") {
                heartbeat_check_interval = std::stoi(value);
                if (heartbeat_check_interval < 1) {
                    std::cerr << "Warning: Invalid heartbeat_check_interval " << heartbeat_check_interval
                              << " at line " << line_number << ". Using default: 10" << std::endl;
                    heartbeat_check_interval = 10;
                    has_errors = true;
                }
            } else {
                std::cerr << "Warning: Unknown configuration key '" << key
                          << "' at line " << line_number << " in " << filename << std::endl;
                has_errors = true;
            }
        } catch (const std::invalid_argument& e) {
            std::cerr << "Error: Invalid value '" << value << "' for key '" << key
                      << "' at line " << line_number << " in " << filename << std::endl;
            has_errors = true;
        } catch (const std::out_of_range& e) {
            std::cerr << "Error: Value out of range '" << value << "' for key '" << key
                      << "' at line " << line_number << " in " << filename << std::endl;
            has_errors = true;
        }
    }

    file.close();

    if (has_errors) {
        std::cerr << "Configuration loaded with warnings/errors. Check values above." << std::endl;
    } else {
        std::cout << "Configuration loaded successfully from " << filename << std::endl;
    }

    return !has_errors;
}

void ServerConfig::parseCommandLine(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            exit(0);
        } else if (arg == "--config" || arg == "-c") {
            if (i + 1 < argc) {
                std::string config_file = argv[++i];
                if (!loadFromFile(config_file)) {
                    std::cerr << "Failed to load configuration from: " << config_file << std::endl;
                }
            } else {
                std::cerr << "Error: --config requires a filename" << std::endl;
                printUsage(argv[0]);
                exit(1);
            }
        } else if (arg == "--port" || arg == "-p") {
            if (i + 1 < argc) {
                try {
                    port = std::stoi(argv[++i]);
                    if (port < 1 || port > 65535) {
                        std::cerr << "Error: Port must be between 1 and 65535" << std::endl;
                        exit(1);
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error: Invalid port number" << std::endl;
                    exit(1);
                }
            } else {
                std::cerr << "Error: --port requires a port number" << std::endl;
                printUsage(argv[0]);
                exit(1);
            }
        } else if (arg == "--ip") {
            if (i + 1 < argc) {
                ip = argv[++i];
            } else {
                std::cerr << "Error: --ip requires an IP address" << std::endl;
                printUsage(argv[0]);
                exit(1);
            }
        } else {
            std::cerr << "Error: Unknown argument: " << arg << std::endl;
            printUsage(argv[0]);
            exit(1);
        }
    }
}

void ServerConfig::printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h, --help              Show this help message" << std::endl;
    std::cout << "  -c, --config FILE       Load configuration from FILE (default: server.conf)" << std::endl;
    std::cout << "  -p, --port PORT         Set server port (overrides config file)" << std::endl;
    std::cout << "  --ip IP                 Set server IP (overrides config file)" << std::endl;
    std::cout << std::endl;
    std::cout << "Default configuration file: server.conf" << std::endl;
}

void ServerConfig::printConfig() {
    std::cout << "=== Server Configuration ===" << std::endl;
    std::cout << "  IP: " << ip << std::endl;
    std::cout << "  Port: " << port << std::endl;
    std::cout << "  Max Rooms: " << max_rooms << std::endl;
    std::cout << "  Max Clients: " << getMaxClients() << " (calculated: max_rooms * 2)" << std::endl;
    std::cout << "  Log File: " << log_file << std::endl;
    std::cout << "  File Logging Enabled: " << (enable_file_logging ? "Yes" : "No") << std::endl;
    std::cout << "  Player Timeout: " << player_timeout_seconds << " seconds" << std::endl;
    std::cout << "  Heartbeat Check Interval: " << heartbeat_check_interval << " seconds" << std::endl;
    std::cout << "============================" << std::endl;
}