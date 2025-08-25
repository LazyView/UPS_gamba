// basic_server.cpp - Protocol-enabled Multi-threaded Server
// KIV/UPS Network Programming Project

#include <iostream>
#include <string>
#include <cstring>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fstream>
#include "protocol.h"

// Server configuration structure
struct ServerConfig {
    std::string ip = "127.0.0.1";
    int port = 8080;
    int max_rooms = 10;
    int max_players_per_room = 6;
    int max_clients = 60;
    int invalid_message_limit = 3;
    std::string log_file = "logs/gamba_server.log";
    bool enable_file_logging = true;

    // Load configuration from file
    bool loadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cout << "⚠️  Config file '" << filename << "' not found, using defaults" << std::endl;
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
        std::cout << "✅ Configuration loaded from: " << filename << std::endl;
        return true;
    }

    // Parse command line arguments
    void parseCommandLine(int argc, char* argv[]) {
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

    // Print usage information
    void printUsage(const char* program_name) {
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

    // Print current configuration
    void printConfig() {
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
};

// Simple player representation
struct Player {
    std::string id;
    std::string name;
    std::string room_id;
    bool connected;

    // Default constructor (required by std::map)
    Player() : id(""), name(""), room_id(""), connected(false) {}

    // Parameterized constructor
    Player(const std::string& player_id, const std::string& player_name)
        : id(player_id), name(player_name), room_id(""), connected(true) {}
};

// Simple room representation
struct Room {
    std::string id;
    std::vector<std::string> players;  // Player IDs
    bool active;

    // Default constructor (required by std::map)
    Room() : id(""), active(false) {}

    // Parameterized constructor
    Room(const std::string& room_id) : id(room_id), active(true) {}
};

class GambaServer {
private:
    ServerConfig config;
    int server_socket;
    std::atomic<bool> running{true};
    std::atomic<int> client_count{0};
    std::atomic<int> player_id_counter{0};

    // Thread-safe data structures
    std::mutex cout_mutex;
    std::mutex players_mutex;
    std::mutex rooms_mutex;

    // Game data
    std::map<std::string, Player> players;       // player_id -> Player
    std::map<std::string, Room> rooms;          // room_id -> Room
    std::map<int, std::string> socket_to_player; // socket -> player_id
    std::map<int, int> client_invalid_count;

    // Thread-safe console output with timestamp
    void log(const std::string& message) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::localtime(&time_t);

        std::cout << "[" << std::put_time(&tm, "%H:%M:%S") << "] " << message << std::endl;
    }

    // Generate unique player ID
    std::string generatePlayerId() {
        int id = ++player_id_counter;
        return "PLAYER_" + std::to_string(id);
    }

    // Generate unique room ID
    std::string generateRoomId() {
        std::lock_guard<std::mutex> lock(rooms_mutex);
        int room_count = rooms.size() + 1;
        return "ROOM_" + std::to_string(room_count);
    }

    // Send protocol message to client
    bool sendMessage(int client_socket, const ProtocolMessage& message) {
        std::string serialized = message.serialize() + "\n";
        int bytes_sent = send(client_socket, serialized.c_str(), serialized.length(), 0);
        return bytes_sent > 0;
    }

    // Handle CONNECT message
    ProtocolMessage handleConnect(const ProtocolMessage& msg, int client_socket) {
        std::string player_name = msg.getData("name", "Anonymous");
        std::string player_id = generatePlayerId();

        // DEBUG: Log what we received
        log("🔍 DEBUG - Raw message data:");
        for (const auto& pair : msg.data) {
            log("🔍   Key: '" + pair.first + "' = Value: '" + pair.second + "'");
        }
        log("🔍 Extracted name: '" + player_name + "'");

        // Add player to server
        {
            std::lock_guard<std::mutex> lock(players_mutex);
            players.emplace(player_id, Player(player_id, player_name));
            socket_to_player[client_socket] = player_id;
        }

        log("✅ Player connected: " + player_name + " (" + player_id + ")");
        return ProtocolHelper::createConnectedResponse(player_id, player_name);
    }

    // Handle JOIN_ROOM message
    ProtocolMessage handleJoinRoom(const ProtocolMessage& msg, int client_socket) {
        std::string player_id = getPlayerIdFromSocket(client_socket);
        if (player_id.empty()) {
            return ProtocolHelper::createErrorResponse("Not connected");
        }

        std::string room_id = msg.room_id;
        if (room_id.empty()) {
            // Create new room
            room_id = generateRoomId();
        }

        // Add player to room
        {
            std::lock_guard<std::mutex> rooms_lock(rooms_mutex);
            std::lock_guard<std::mutex> players_lock(players_mutex);

            // Create room if it doesn't exist
            if (rooms.find(room_id) == rooms.end()) {
                rooms[room_id] = Room(room_id);
                log("🏠 Created room: " + room_id);
            }

            // Add player to room
            auto& room = rooms[room_id];
            room.players.push_back(player_id);

            // Update player's room
            players[player_id].room_id = room_id;
        }

        log("🚪 Player " + player_id + " joined room " + room_id);
        return ProtocolHelper::createRoomJoinedResponse(player_id, room_id);
    }

    // Handle PING message
    ProtocolMessage handlePing(const ProtocolMessage& /* msg */) {  // Add comment to suppress warning
        return ProtocolHelper::createPongResponse();
    }

    // Get player ID from socket
    std::string getPlayerIdFromSocket(int client_socket) {
        std::lock_guard<std::mutex> lock(players_mutex);
        auto it = socket_to_player.find(client_socket);
        return (it != socket_to_player.end()) ? it->second : "";
    }

    // Handle client disconnection
    void handleDisconnect(int client_socket) {
        std::string player_id = getPlayerIdFromSocket(client_socket);
        if (!player_id.empty()) {
            std::lock_guard<std::mutex> lock(players_mutex);
            if (players.find(player_id) != players.end()) {
                log("👋 Player disconnected: " + players[player_id].name + " (" + player_id + ")");
                players.erase(player_id);
            }
            socket_to_player.erase(client_socket);
        }
        // Clean up invalid message counter
        {
            std::lock_guard<std::mutex> lock(players_mutex);
            client_invalid_count.erase(client_socket);  // Add this line
        }
    }

    // Process protocol message
    ProtocolMessage processMessage(const std::string& raw_message, int client_socket) {
        // Remove newline if present
        log("🔍 RAW MESSAGE (length=" + std::to_string(raw_message.length()) + "): '" + raw_message + "'");
        std::string clean_message = raw_message;
        if (!clean_message.empty() && clean_message.back() == '\n') {
            clean_message.pop_back();
        }
        if (!clean_message.empty() && clean_message.back() == '\r') {
            clean_message.pop_back();
        }
        log("🔍 CLEAN MESSAGE (length=" + std::to_string(clean_message.length()) + "): '" + clean_message + "'");
        // Validate message format
        if (!ProtocolHelper::isValidMessage(clean_message)) {
            if (!handleInvalidMessage(client_socket, "Invalid format: " + clean_message)) {
                // Return special disconnect signal
                ProtocolMessage disconnect_msg(MessageType::ERROR_MSG);
                disconnect_msg.setData("disconnect", "true");
                return disconnect_msg;
            }
            return ProtocolHelper::createErrorResponse("Invalid message format");
        }

        // Parse message
        ProtocolMessage msg = ProtocolMessage::parse(clean_message);

        log("📨 Received " + ProtocolHelper::getMessageTypeName(msg.type) + " from socket " + std::to_string(client_socket));

        // Process based on message type
        switch (msg.type) {
            case MessageType::CONNECT:
                return handleConnect(msg, client_socket);

            case MessageType::JOIN_ROOM:
                return handleJoinRoom(msg, client_socket);

            case MessageType::PING:
                return handlePing(msg);

            default:
                log("⚠️  Unhandled message type: " + ProtocolHelper::getMessageTypeName(msg.type));
                return ProtocolHelper::createErrorResponse("Unknown message type");
        }
    }

    // Handle individual client in separate thread
    void handleClient(int client_socket, std::string client_ip, int client_port) {
        int client_num = ++client_count;
        log("🎉 Client #" + std::to_string(client_num) + " connected from "
            + client_ip + ":" + std::to_string(client_port));

        char buffer[1024];

        while (running) {
            // Receive message from client
            int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

            if (bytes_received <= 0) {
                if (bytes_received == 0) {
                    log("👋 Client #" + std::to_string(client_num) + " disconnected gracefully");
                } else {
                    log("❌ Client #" + std::to_string(client_num) + " connection error");
                }
                break;
            }

            // Null-terminate received data
            buffer[bytes_received] = '\0';
            std::string raw_message(buffer);

            // Process protocol message
            ProtocolMessage response = processMessage(raw_message, client_socket);

            // Check if we should disconnect
            if (response.type == MessageType::ERROR_MSG && response.hasData("disconnect")) {
                log("Disconnecting client due to too many invalid messages");
                break;  // Exit the client loop
            }

            // Send response
            if (!sendMessage(client_socket, response)) {
                log("❌ Failed to send response to Client #" + std::to_string(client_num));
                break;
            }

            log("📤 Sent " + ProtocolHelper::getMessageTypeName(response.type) + " response to Client #" + std::to_string(client_num));
        }

        // Handle disconnection
        handleDisconnect(client_socket);

        // Cleanup
        close(client_socket);
        --client_count;
        log("🔒 Client #" + std::to_string(client_num) + " connection closed. Active: " + std::to_string(client_count.load()));
    }

    bool handleInvalidMessage(int client_socket, const std::string& reason) {
        std::lock_guard<std::mutex> lock(players_mutex);

        client_invalid_count[client_socket]++;
        int count = client_invalid_count[client_socket];

        log("Invalid message from socket " + std::to_string(client_socket) +
            " (count: " + std::to_string(count) + "/" +
            std::to_string(config.invalid_message_limit) + "): " + reason);

        if (count >= config.invalid_message_limit) {
            log("Disconnecting socket " + std::to_string(client_socket) +
                " - exceeded invalid message limit");
            return false;  // Signal to disconnect
        }

        return true;  // Continue connection
    }

public:
    GambaServer(const ServerConfig& cfg) : config(cfg), server_socket(-1) {}

    ~GambaServer() {
        stop();
    }

    bool start() {  // Remove port parameter
        log("=== GAMBA GAME SERVER (Protocol-Enabled) ===");
        log("Starting server on " + config.ip + ":" + std::to_string(config.port) + "...");

        // Create TCP socket
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket == -1) {
            log("Failed to create socket");
            return false;
        }
        log("Socket created successfully");

        // Allow socket reuse
        int opt = 1;
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        // Configure server address using config
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(config.port);  // Use config.port

        // Bind socket
        if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            log("Failed to bind socket to port " + std::to_string(config.port));
            close(server_socket);
            return false;
        }
        log("Socket bound to port " + std::to_string(config.port));

        // Start listening
        if (listen(server_socket, 10) < 0) {
            log("❌ Failed to listen on socket");
            close(server_socket);
            return false;
        }

        log("🚀 Server ready - Protocol enabled!");
        log("📋 Supported messages: CONNECT, JOIN_ROOM, PING");
        log("💡 Test with: telnet localhost " + std::to_string(config.port));
        log("💡 Example: 0||name=Alice");

        return true;
    }

    void run() {
        while (running) {
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);

            int client_socket = accept(server_socket,
                                     (struct sockaddr*)&client_addr,
                                     &client_addr_len);

            if (client_socket < 0) {
                if (running) {
                    log("⚠️  Failed to accept client connection");
                }
                continue;
            }

            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
            int client_port = ntohs(client_addr.sin_port);

            std::thread client_thread(&GambaServer::handleClient, this,
                                    client_socket, std::string(client_ip), client_port);
            client_thread.detach();

            log("👥 Active connections: " + std::to_string(client_count.load()));
        }
    }

    void stop() {
        if (running) {
            running = false;
            if (server_socket != -1) {
                close(server_socket);
                server_socket = -1;
            }
            log("🛑 Server stopped");
        }
    }
};

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