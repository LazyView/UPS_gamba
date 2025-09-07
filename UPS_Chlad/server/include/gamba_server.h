// gamba_server.h - Main Game Server Class
// KIV/UPS Network Programming Project

#ifndef GAMBA_SERVER_H
#define GAMBA_SERVER_H

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

#include "server_config.h"
#include "game_logic.h"
#include "protocol.h"

// Simple player representation
struct Player {
    std::string id;
    std::string name;
    std::string room_id;
    bool connected;
    std::chrono::steady_clock::time_point last_ping;
    std::chrono::steady_clock::time_point disconnection_start;
    bool temporarily_disconnected = false;

    Player();
    Player(const std::string& player_id, const std::string& player_name);
};

// Room representation
struct Room {
    std::string id;
    std::vector<std::string> players;
    bool active;
    GambaGameState game_state;

    Room();
    Room(const std::string& room_id);
};

// Main server class
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
    std::map<std::string, Player> players;
    std::map<std::string, Room> rooms;
    std::map<int, std::string> socket_to_player;
    std::map<int, int> client_invalid_count;

    // Private methods
    void log(const std::string& message);
    std::string generatePlayerId();
    std::string generateRoomId();
    bool sendMessage(int client_socket, const ProtocolMessage& message);

	// Heartbeat check
	std::map<std::string, std::chrono::steady_clock::time_point> player_last_ping;
	std::mutex heartbeat_mutex;
	std::thread heartbeat_thread;

    // Protocol handlers
    ProtocolMessage handleConnect(const ProtocolMessage& msg, int client_socket);
    ProtocolMessage handleReconnect(const ProtocolMessage& msg, int client_socket);
    ProtocolMessage handleJoinRoom(const ProtocolMessage& msg, int client_socket);
    ProtocolMessage handlePing(const ProtocolMessage& msg);
    ProtocolMessage handleStartGame(const ProtocolMessage& msg, int client_socket);
    ProtocolMessage handlePlayCards(const ProtocolMessage& msg, int client_socket);
    ProtocolMessage handlePickupPile(const ProtocolMessage& msg, int client_socket);

    // Utility methods
    std::string getPlayerIdFromSocket(int client_socket);
    void handleDisconnect(int client_socket);
    ProtocolMessage processMessage(const std::string& raw_message, int client_socket);
    void handleClient(int client_socket, std::string client_ip, int client_port);
    bool handleInvalidMessage(int client_socket, const std::string& reason);
	void startHeartbeatMonitor();
	void checkHeartbeats();
	void pauseGameInRoom(const std::string& room_id, const std::string& disconnected_player);
	void resumeGameInRoom(const std::string& room_id, const std::string& reconnected_player);
    void broadcastGameState(const std::string& room_id);

public:
    GambaServer(const ServerConfig& cfg);
    ~GambaServer();

    bool start();
    void run();
    void stop();
};

#endif // GAMBA_SERVER_H