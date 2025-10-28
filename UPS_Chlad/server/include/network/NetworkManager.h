//
// Created by chlad on 9/15/2025.
//

#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <atomic>
#include <string>
#include <condition_variable>
#include <mutex>

// Forward declarations
class ProtocolMessage;
class PlayerManager;
class RoomManager;
class MessageHandler;
class MessageValidator;
class Logger;
struct ServerConfig;

class NetworkManager {
private:
    int server_socket;
    std::atomic<bool> running;
    std::string server_ip;
    int server_port;

    // Manager pointers:
    PlayerManager* playerManager;
    RoomManager* roomManager;
    MessageHandler* messageHandler;
    MessageValidator* validator;
    Logger* logger;
    const ServerConfig* config;

    // Heartbeat monitoring
    std::thread heartbeat_monitor_thread;
    std::atomic<bool> heartbeat_running;
    std::condition_variable heartbeat_cv;
    std::mutex heartbeat_mutex;

public:
	NetworkManager(PlayerManager* pm, RoomManager* rm, MessageHandler* mh,
               MessageValidator* mv, Logger* lg, const ServerConfig* cfg,
               const std::string& ip, int port);

    ~NetworkManager();

    bool start();        // Create and bind socket
    void run();          // Main accept loop
    void stop();         // Stop server gracefully
    /**
    * Broadcasting to all players in the room. Retrieves all room players and tries to send message to all of them except the requester.
    * Validates the count of sent messages against number of players in the room - 1
    * @param room_id - room to broadcast to
    * @param message - message to be broadcasted
    * @param excluded_player - requester that wont receive broadcasted message
    */
    void broadcastToRoom(const std::string& room_id, const ProtocolMessage& message, const std::string& exclude_player = "");

private:

    bool setupSocket();                    // Create, bind, listen
    void handleClient(int client_socket);  // Process one client
    void cleanup();                        // Clean shutdown

    // Heartbeat monitoring
    /*
    * Inicializes heartbeat for u Player
    */
    void startHeartbeatMonitor();          // Start heartbeat monitoring thread
    /*
    * stop Heartbeat monitoring
    */
    void stopHeartbeatMonitor();           // Stop heartbeat monitoring thread
    /*
    * Loop that takes care of players heartbeat, if server didnt receive ping for a 60s, marks player as afk (short-term).
    * After 120s removes player from game and server (long-term).
    */
    void heartbeatMonitorLoop();           // Main heartbeat monitoring loop
};

#endif //NETWORKMANAGER_H