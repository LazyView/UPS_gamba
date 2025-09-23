//
// Created by chlad on 9/15/2025.
//

#include "NetworkManager.h"
#include "core/PlayerManager.h"
#include "core/RoomManager.h"
#include "network/MessageHandler.h"
#include "network/MessageValidator.h"
#include "core/Logger.h"
#include "core/server_config.h"
#include "protocol/ProtocolMessage.h"
#include <errno.h>
#include <cstring>
#include <vector>
#include <algorithm>
#include <fcntl.h>
#include <chrono>
#include <sys/socket.h>

NetworkManager::NetworkManager(PlayerManager* pm, RoomManager* rm, MessageHandler* mh,
                               MessageValidator* mv, Logger* lg, const ServerConfig* cfg,
                               const std::string& ip, int port)
    : server_socket(-1), running(false), server_ip(ip), server_port(port),
      playerManager(pm), roomManager(rm), messageHandler(mh), validator(mv), logger(lg), config(cfg),
      heartbeat_running(false) {

    if (!playerManager || !roomManager || !messageHandler || !validator || !logger || !config) {
        throw std::invalid_argument("NetworkManager: All manager pointers must be non-null");
    }

    logger->info("NetworkManager initialized with IP: " + ip + ", Port: " + std::to_string(port));
}

NetworkManager::~NetworkManager() {
    if (running.load()) {
        logger->warning("NetworkManager destructor called while still running - forcing stop");
        stop();
    }
    stopHeartbeatMonitor();
    cleanup();
    logger->info("NetworkManager destroyed");
}

bool NetworkManager::start() {
    if (running.load()) {
        logger->warning("NetworkManager::start() called but server is already running");
        return false;
    }

    logger->info("Starting NetworkManager server...");

    if (!setupSocket()) {
        logger->error("Failed to setup server socket");
        return false;
    }

    running.store(true);

    // Start heartbeat monitoring
    startHeartbeatMonitor();

    logger->info("NetworkManager started successfully on " + server_ip + ":" + std::to_string(server_port));
    return true;
}

void NetworkManager::run() {
    if (!running.load()) {
        logger->error("NetworkManager::run() called but server is not started");
        return;
    }

    logger->info("NetworkManager entering main accept loop");

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    std::vector<std::thread> client_threads;

    while (running.load()) {
        // Accept new client connection
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);

        if (client_socket < 0) {
            if (running.load()) {
                logger->error("Accept failed: " + std::string(strerror(errno)));
            }
            continue;
        }

        // Log client connection
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        logger->info("New client connected from " + std::string(client_ip) + ":" + std::to_string(ntohs(client_addr.sin_port)) + " (socket: " + std::to_string(client_socket) + ")");

        // Create new thread to handle client
        try {
            client_threads.emplace_back(&NetworkManager::handleClient, this, client_socket);
            client_threads.back().detach(); // Detach thread for independent execution
        } catch (const std::exception& e) {
            logger->error("Failed to create thread for client " + std::to_string(client_socket) + ": " + e.what());
            close(client_socket);
        }
    }

    logger->info("NetworkManager exiting main accept loop");
}

void NetworkManager::stop() {
    if (!running.load()) {
        logger->warning("NetworkManager::stop() called but server is not running");
        return;
    }

    logger->info("Stopping NetworkManager...");
    running.store(false);

    // Stop heartbeat monitoring
    stopHeartbeatMonitor();

    // Close server socket to break accept() loop
    if (server_socket >= 0) {
        close(server_socket);
        server_socket = -1;
    }

    cleanup();
    logger->info("NetworkManager stopped");
}

bool NetworkManager::setupSocket() {
    // Create TCP socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        logger->error("Failed to create socket: " + std::string(strerror(errno)));
        return false;
    }

    // Set socket options for reuse
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        logger->error("Failed to set SO_REUSEADDR: " + std::string(strerror(errno)));
        close(server_socket);
        server_socket = -1;
        return false;
    }

    // Setup server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0) {
        logger->error("Invalid IP address: " + server_ip);
        close(server_socket);
        server_socket = -1;
        return false;
    }

    // Bind socket
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        logger->error("Failed to bind socket to " + server_ip + ":" + std::to_string(server_port) + ": " + std::string(strerror(errno)));
        close(server_socket);
        server_socket = -1;
        return false;
    }

    // Start listening
    const int backlog = 128; // Maximum pending connections
    if (listen(server_socket, backlog) < 0) {
        logger->error("Failed to listen on socket: " + std::string(strerror(errno)));
        close(server_socket);
        server_socket = -1;
        return false;
    }

    logger->info("Socket setup complete - listening on " + server_ip + ":" + std::to_string(server_port));
    return true;
}

void NetworkManager::handleClient(int client_socket) {
    const size_t BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE];
    std::string message_buffer;

    logger->debug("Client handler started for socket " + std::to_string(client_socket));

    try {
        while (running.load()) {
            // Receive data from client
            ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);

            if (bytes_received <= 0) {
                if (bytes_received == 0) {
                    logger->info("Client " + std::to_string(client_socket) + " disconnected gracefully");
                } else {
                    logger->warning("Receive error from client " + std::to_string(client_socket) + ": " + std::string(strerror(errno)));
                }
                break;
            }

            // Null-terminate and add to buffer
            buffer[bytes_received] = '\0';
            message_buffer += buffer;

            // Process complete messages (assuming newline-delimited)
            size_t pos = 0;
            while ((pos = message_buffer.find('\n')) != std::string::npos) {
                std::string complete_message = message_buffer.substr(0, pos);
                message_buffer.erase(0, pos + 1);

                // Remove carriage return if present
                if (!complete_message.empty() && complete_message.back() == '\r') {
                    complete_message.pop_back();
                }

                if (!complete_message.empty()) {
                    logger->debug("Received message from client " + std::to_string(client_socket) + ": " + complete_message);

                    // Process message through MessageHandler
                    try {
                        ProtocolMessage response = messageHandler->processMessage(complete_message, client_socket);

                        // Send response back to client
                        std::string response_str = response.serialize() + "\n";
                        ssize_t bytes_sent = send(client_socket, response_str.c_str(), response_str.length(), MSG_NOSIGNAL);

                        if (bytes_sent < 0) {
                            logger->error("Failed to send response to client " + std::to_string(client_socket) + ": " + std::string(strerror(errno)));
                            break;
                        } else if (static_cast<size_t>(bytes_sent) != response_str.length()) {
                            logger->warning("Partial send to client " + std::to_string(client_socket) + ": sent " + std::to_string(bytes_sent) + "/" + std::to_string(response_str.length()) + " bytes");
                        } else {
                            logger->debug("Sent response to client " + std::to_string(client_socket) + ": " + response.serialize());
                        }

                        // Check if client should be disconnected
                        if (response.hasData("disconnect") && response.getData("disconnect") == "true") {
                            logger->info("Disconnecting client " + std::to_string(client_socket) + " as requested");
                            // Immediately shutdown socket to prevent further communication
                            shutdown(client_socket, SHUT_RDWR);
                            return; // Exit the entire handleClient function immediately
                        }

                    } catch (const std::exception& e) {
                        logger->error("Error processing message from client " + std::to_string(client_socket) + ": " + e.what());

                        // Send error response
                        ProtocolMessage error_response(MessageType::ERROR_MSG);
                        error_response.setData("message", "Internal server error");
                        std::string error_str = error_response.serialize() + "\n";
                        send(client_socket, error_str.c_str(), error_str.length(), MSG_NOSIGNAL);
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        logger->error("Exception in client handler for socket " + std::to_string(client_socket) + ": " + e.what());
    }

    // Clean up client connection
    logger->info("Cleaning up client " + std::to_string(client_socket));

    // Remove player from manager if they were connected
    try {
        playerManager->removePlayerBySocket(client_socket);
    } catch (const std::exception& e) {
        logger->warning("Error removing player for socket " + std::to_string(client_socket) + ": " + e.what());
    }

    // Close socket
    if (close(client_socket) < 0) {
        logger->warning("Error closing client socket " + std::to_string(client_socket) + ": " + std::string(strerror(errno)));
    }

    logger->debug("Client handler finished for socket " + std::to_string(client_socket));
}

void NetworkManager::cleanup() {
    // Close server socket if still open
    if (server_socket >= 0) {
        logger->debug("Closing server socket");
        if (close(server_socket) < 0) {
            logger->warning("Error closing server socket: " + std::string(strerror(errno)));
        }
        server_socket = -1;
    }

    // Additional cleanup could include:
    // - Waiting for client threads to finish (if we kept track of them)
    // - Cleaning up any other resources

    logger->debug("NetworkManager cleanup complete");
}

void NetworkManager::startHeartbeatMonitor() {
    if (heartbeat_running.load()) {
        logger->warning("Heartbeat monitor already running");
        return;
    }

    heartbeat_running.store(true);
    heartbeat_monitor_thread = std::thread(&NetworkManager::heartbeatMonitorLoop, this);
    logger->info("Heartbeat monitor started with " + std::to_string(config->player_timeout_seconds)
                + "s timeout and " + std::to_string(config->heartbeat_check_interval) + "s check interval");
}

void NetworkManager::stopHeartbeatMonitor() {
    if (!heartbeat_running.load()) {
        return;
    }

    logger->info("Stopping heartbeat monitor...");
    heartbeat_running.store(false);

    // Notify the condition variable to wake up the heartbeat thread
    {
        std::lock_guard<std::mutex> lock(heartbeat_mutex);
        heartbeat_cv.notify_one();
    }

    if (heartbeat_monitor_thread.joinable()) {
        heartbeat_monitor_thread.join();
    }

    logger->info("Heartbeat monitor stopped");
}

void NetworkManager::heartbeatMonitorLoop() {
    logger->debug("Heartbeat monitor thread started");

    while (heartbeat_running.load()) {
        try {
            // Get list of timed out players
            std::vector<std::string> timed_out_players = playerManager->getTimedOutPlayers(config->player_timeout_seconds);

            // Process each timed out player
            for (const std::string& player_name : timed_out_players) {
                logger->info("Player '" + player_name + "' timed out - marking as disconnected");

                // Get player's room before disconnecting
                std::string room_id = playerManager->getPlayerRoom(player_name);

                // Handle timeout in room context (remove from room, handle game state)
                roomManager->handlePlayerTimeout(player_name);

                // Mark player as disconnected
                playerManager->markPlayerDisconnected(player_name);

                if (!room_id.empty() && room_id != "lobby") {
                    logger->info("Player '" + player_name + "' was removed from room '" + room_id + "' due to timeout");
                }
            }

            if (!timed_out_players.empty()) {
                logger->info("Processed " + std::to_string(timed_out_players.size()) + " timed out players");
            }

        } catch (const std::exception& e) {
            logger->error("Exception in heartbeat monitor: " + std::string(e.what()));
        }

        // Use condition variable with timeout instead of blocking sleep
        // This allows immediate wake-up during shutdown
        std::unique_lock<std::mutex> lock(heartbeat_mutex);
        heartbeat_cv.wait_for(lock, std::chrono::seconds(config->heartbeat_check_interval),
                              [this] { return !heartbeat_running.load(); });
    }

    logger->debug("Heartbeat monitor thread stopped");
}