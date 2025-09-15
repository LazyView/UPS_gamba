//
// Created by chlad on 9/15/2025.
//

#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

// TODO: Add necessary includes
// - Socket headers (sys/socket.h, netinet/in.h, arpa/inet.h, unistd.h)
// - Threading (thread, atomic)
// - String for IP address
// - Forward declarations for your managers

// Forward declarations
class PlayerManager;
class RoomManager;
class MessageHandler;
class MessageValidator;
class Logger;

class NetworkManager {
private:
    // TODO: Add private members
    // - int server_socket;
    // - std::atomic<bool> running;
    // - std::string server_ip;
    // - int server_port;
    // - Pointers to managers (PlayerManager*, RoomManager*, etc.)

public:
    // TODO: Constructor - takes managers as parameters
    NetworkManager(/* parameters for managers, ip, port */);

    // TODO: Destructor
    ~NetworkManager();

    // TODO: Main server methods
    bool start();        // Create and bind socket
    void run();          // Main accept loop
    void stop();         // Stop server gracefully

private:
    // TODO: Helper methods
    bool setupSocket();                    // Create, bind, listen
    void handleClient(int client_socket);  // Process one client
    void cleanup();                        // Clean shutdown
};

#endif //NETWORKMANAGER_H