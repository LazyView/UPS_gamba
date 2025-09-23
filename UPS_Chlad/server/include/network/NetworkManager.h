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

private:

    bool setupSocket();                    // Create, bind, listen
    void handleClient(int client_socket);  // Process one client
    void cleanup();                        // Clean shutdown

    // Heartbeat monitoring
    void startHeartbeatMonitor();          // Start heartbeat monitoring thread
    void stopHeartbeatMonitor();           // Stop heartbeat monitoring thread
    void heartbeatMonitorLoop();           // Main heartbeat monitoring loop
};

#endif //NETWORKMANAGER_H