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

        // Create and detach thread for handling client
        try {
            std::thread client_handler(&NetworkManager::handleClient, this, client_socket);
            client_handler.detach();
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

    // Stop heartbeat monitoring first
    stopHeartbeatMonitor();

    // Close server socket to break accept() loop
    if (server_socket >= 0) {
        logger->debug("Closing server socket to break accept loop");
        shutdown(server_socket, SHUT_RDWR);
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
    const int backlog = 128;
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
    const size_t MAX_MESSAGE_SIZE = 8192;
    char buffer[BUFFER_SIZE];
    std::string message_buffer;

    logger->debug("Client handler started for socket " + std::to_string(client_socket));

    try {
        while (running.load()) {
            // Receive data from client
            ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);

            logger->debug("recv() returned: " + std::to_string(bytes_received) + " bytes for socket " + std::to_string(client_socket));

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

            // Protect against huge messages
            if (message_buffer.size() > MAX_MESSAGE_SIZE) {
                logger->warning("Message too large from client " + std::to_string(client_socket) + ", disconnecting");
                break;
            }
            
            logger->debug("Current message_buffer for socket " + std::to_string(client_socket) + ": '" + message_buffer + "'");

            // Process complete messages (newline-delimited)
            size_t pos = 0;
            while ((pos = message_buffer.find('\n')) != std::string::npos) {
                std::string complete_message = message_buffer.substr(0, pos);
                message_buffer.erase(0, pos + 1);

                // Remove carriage return if present
                if (!complete_message.empty() && complete_message.back() == '\r') {
                    complete_message.pop_back();
                }

                logger->debug("Processing complete message: '" + complete_message + "'");

                if (!complete_message.empty()) {
                    logger->debug("Received message from client " + std::to_string(client_socket) + ": " + complete_message);

                    // Process message through MessageHandler - NOW RETURNS VECTOR
                    try {
                        std::vector<ProtocolMessage> responses = messageHandler->processMessage(complete_message, client_socket);
                        
                        logger->debug("MessageHandler returned " + std::to_string(responses.size()) + " response(s)");

                        // Process each response
                        for (const ProtocolMessage& response : responses) {
                            logger->debug("Processing response type: " + std::to_string(static_cast<int>(response.getType())));
                            
                            if (response.should_broadcast_to_room) {
                                // Handle broadcast messages
                                std::string room_id = response.getRoomId();
                                
                                if (!room_id.empty()) {
                                    std::string player_name = playerManager->getPlayerIdFromSocket(client_socket);
                                    
                                    // STEP 1: Send original response to requesting client
                                    std::string response_str = response.serialize() + "\n";
                                    ssize_t bytes_sent = send(client_socket, response_str.c_str(), response_str.length(), MSG_NOSIGNAL);
                                    
                                    if (bytes_sent < 0) {
                                        logger->error("Failed to send response to requesting client " + std::to_string(client_socket) + ": " + std::string(strerror(errno)));
                                    } else if (static_cast<size_t>(bytes_sent) != response_str.length()) {
                                        logger->warning("Partial send to requesting client " + std::to_string(client_socket));
                                    } else {
                                        logger->debug("Sent response to requesting client " + std::to_string(client_socket));
                                    }
                                    
                                    // STEP 2: Broadcast modified version to OTHER players (exclude sender)
                                    logger->debug("Broadcasting to room " + room_id + " (excluding " + player_name + ")");
                                    
                                    ProtocolMessage broadcast_msg = response;
                                    broadcast_msg.setData("broadcast_type", "room_notification");
                                    
                                    // Add context about who triggered the action
                                    if (response.getType() == MessageType::ROOM_JOINED) {
                                        broadcast_msg.setData("joined_player", player_name);
                                    }
                                    
                                    broadcastToRoom(room_id, broadcast_msg, player_name);
                                } else {
                                    logger->warning("Broadcast flagged but no room_id in response");
                                }
                                
                            } else if (!response.player_id.empty()) {
                                // Message targeted at specific player (not the sender)
                                logger->debug("Sending targeted message to player '" + response.player_id + "'");
                                
                                auto player_opt = playerManager->getPlayer(response.player_id);
                                
                                if (player_opt.has_value() && player_opt->connected && player_opt->socket_fd != -1) {
                                    std::string response_str = response.serialize() + "\n";
                                    int target_socket = player_opt->socket_fd;
                                    
                                    ssize_t bytes_sent = send(target_socket, response_str.c_str(), response_str.length(), MSG_NOSIGNAL);
                                    
                                    if (bytes_sent < 0) {
                                        logger->error("Failed to send targeted message to player '" + response.player_id + "': " + std::string(strerror(errno)));
                                    } else if (static_cast<size_t>(bytes_sent) != response_str.length()) {
                                        logger->warning("Partial send to player '" + response.player_id + "': sent " + std::to_string(bytes_sent) + "/" + std::to_string(response_str.length()) + " bytes");
                                    } else {
                                        logger->debug("Sent targeted message to player '" + response.player_id + "' on socket " + std::to_string(target_socket));
                                    }
                                } else {
                                    logger->warning("Cannot send to player '" + response.player_id + "' - disconnected or invalid socket");
                                }
                                
                            } else {
                                // Regular response to the requesting client
                                std::string response_str = response.serialize() + "\n";
                                
                                logger->debug("Sending response: '" + response_str + "'");
                                
                                ssize_t bytes_sent = send(client_socket, response_str.c_str(), response_str.length(), MSG_NOSIGNAL);
                                
                                if (bytes_sent < 0) {
                                    logger->error("Failed to send response to client " + std::to_string(client_socket) + ": " + std::string(strerror(errno)));
                                    break;
                                } else if (static_cast<size_t>(bytes_sent) != response_str.length()) {
                                    logger->warning("Partial send to client " + std::to_string(client_socket) + ": sent " + std::to_string(bytes_sent) + "/" + std::to_string(response_str.length()) + " bytes");
                                } else {
                                    logger->debug("Sent response to client " + std::to_string(client_socket) + ": " + response.serialize());
                                }
                            }
                            
                            // Check if client should be disconnected
                            if (response.hasData("disconnect") && response.getData("disconnect") == "true") {
                                logger->info("Disconnecting client " + std::to_string(client_socket) + " due to invalid message");
                                
                                // Mark player as temporarily disconnected before closing socket
                                std::string disconnected_player = playerManager->getPlayerIdFromSocket(client_socket);
                                if (!disconnected_player.empty()) {
                                    std::string room_id = playerManager->getPlayerRoom(disconnected_player);
                                    playerManager->markPlayerTemporarilyDisconnected(disconnected_player);
                                    
                                    // Broadcast disconnection to room
                                    if (!room_id.empty()) {
                                        ProtocolMessage disconnect_broadcast(MessageType::PLAYER_DISCONNECTED);
                                        disconnect_broadcast.player_id = disconnected_player;
                                        disconnect_broadcast.room_id = room_id;
                                        disconnect_broadcast.setData("disconnected_player", disconnected_player);
                                        disconnect_broadcast.setData("status", "invalid_message");
                                        
                                        broadcastToRoom(room_id, disconnect_broadcast, disconnected_player);
                                    }
                                }
                                
                                // Remove socket mapping
                                playerManager->removeSocketMapping(client_socket);
                                
                                // Close socket and exit
                                shutdown(client_socket, SHUT_RDWR);
                                close(client_socket);
                                return;  // Exit handleClient immediately
                            }
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
    
    // Cleanup after client disconnects
    logger->info("Client " + std::to_string(client_socket) + " disconnected, marking for reconnection window");

    // Get player name before removing socket mapping
    std::string disconnected_player = playerManager->getPlayerIdFromSocket(client_socket);
    if (!disconnected_player.empty()) {
        // Get room before marking disconnected
        std::string room_id = playerManager->getPlayerRoom(disconnected_player);
        playerManager->markPlayerTemporarilyDisconnected(disconnected_player);
        
        // Broadcast disconnection to room
        if (!room_id.empty()) {
            ProtocolMessage disconnect_broadcast(MessageType::PLAYER_DISCONNECTED);
            disconnect_broadcast.player_id = disconnected_player;
            disconnect_broadcast.room_id = room_id;
            disconnect_broadcast.setData("disconnected_player", disconnected_player);
            disconnect_broadcast.setData("status", "temporarily_disconnected");

            broadcastToRoom(room_id, disconnect_broadcast, disconnected_player);
        }
    } else {
        logger->debug("No player found for disconnected socket " + std::to_string(client_socket));
    }

    // Remove socket mapping
    playerManager->removeSocketMapping(client_socket);

    // Close the socket
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
            // Get timed out players (normal ping timeout)
            std::vector<std::string> timed_out_players = playerManager->getTimedOutPlayers(config->player_timeout_seconds);

            // Get players who need cleanup (2-minute reconnection window expired)
            std::vector<std::string> cleanup_players = playerManager->getDisconnectedPlayersForCleanup(120);

            // Handle ping timeouts (mark as temporarily disconnected)
            for (const std::string& player_name : timed_out_players) {
                logger->info("Player '" + player_name + "' timed out - marking as temporarily disconnected");
                std::string room_id = playerManager->getPlayerRoom(player_name);

                playerManager->markPlayerTemporarilyDisconnected(player_name);

                // Broadcast timeout to room
                if (!room_id.empty()) {
                    ProtocolMessage timeout_broadcast(MessageType::PLAYER_DISCONNECTED);
                    timeout_broadcast.player_id = player_name;
                    timeout_broadcast.room_id = room_id;
                    timeout_broadcast.setData("disconnected_player", player_name);
                    timeout_broadcast.setData("status", "timed_out");

                    broadcastToRoom(room_id, timeout_broadcast, player_name);
                }
            }

            // Handle reconnection window expiry (complete cleanup)
            for (const std::string& player_name : cleanup_players) {
                logger->info("Player '" + player_name + "' reconnection window expired - cleaning up");

                std::string room_id = playerManager->getPlayerRoom(player_name);
                
                if (!room_id.empty() && room_id != "lobby") {
                    // Get remaining players before cleanup
                    std::vector<std::string> room_players = roomManager->getRoomPlayers(room_id);
                    
                    // Check if game was active
                    bool game_was_active = false;
                    roomManager->withRoom(room_id, [&](Room* room) -> bool {
                        if (room) {
                            game_was_active = room->isGameActive();
                        }
                        return true;
                    });
                    
                    // Remove the disconnected player from room
                    roomManager->handlePlayerTimeout(player_name, room_id);
                    
                    // If game was active, notify remaining players and end the game
                    if (game_was_active) {
                        for (const std::string& remaining_player : room_players) {
                            if (remaining_player != player_name) {
                                logger->info("Notifying player '" + remaining_player + "' that game ended due to opponent timeout");
                                
                                // Get player socket
                                auto player_opt = playerManager->getPlayer(remaining_player);
                                if (player_opt.has_value() && player_opt->connected && player_opt->socket_fd != -1) {
                                    // Send GAME_OVER message
                                    ProtocolMessage game_over(MessageType::GAME_OVER);
                                    game_over.player_id = remaining_player;
                                    game_over.room_id = room_id;
                                    game_over.setData("winner", remaining_player);
                                    game_over.setData("reason", "opponent_disconnect");
                                    game_over.setData("status", "game_over");
                                    
                                    std::string game_over_msg = game_over.serialize() + "\n";
                                    send(player_opt->socket_fd, game_over_msg.c_str(), game_over_msg.length(), MSG_NOSIGNAL);
                                    
                                    // Send ROOM_LEFT message (back to lobby)
                                    ProtocolMessage room_left(MessageType::ROOM_LEFT);
                                    room_left.player_id = remaining_player;
                                    room_left.room_id = "";
                                    room_left.setData("status", "left");
                                    
                                    std::string room_left_msg = room_left.serialize() + "\n";
                                    send(player_opt->socket_fd, room_left_msg.c_str(), room_left_msg.length(), MSG_NOSIGNAL);
                                    
                                    // Clear player's room assignment
                                    playerManager->clearPlayerRoom(remaining_player);
                                    
                                    logger->info("Player '" + remaining_player + "' returned to lobby after opponent timeout");
                                }
                            }
                        }
                        
                        // Delete the room
                        roomManager->deleteRoom(room_id);
                        logger->info("Room '" + room_id + "' deleted after long-term disconnection");
                    }
                }
                
                // Remove disconnected player
                playerManager->clearPlayerRoom(player_name);
                playerManager->removePlayer(player_name);
            }

            if (!timed_out_players.empty() || !cleanup_players.empty()) {
                logger->info("Processed " + std::to_string(timed_out_players.size()) + " timeouts and " + std::to_string(cleanup_players.size()) + " cleanups");
            }

        } catch (const std::exception& e) {
            logger->error("Exception in heartbeat monitor: " + std::string(e.what()));
        }

        // Use condition variable with timeout
        std::unique_lock<std::mutex> lock(heartbeat_mutex);
        heartbeat_cv.wait_for(lock, std::chrono::seconds(config->heartbeat_check_interval),
                              [this] { return !heartbeat_running.load(); });
    }

    logger->debug("Heartbeat monitor thread stopped");
}

void NetworkManager::broadcastToRoom(const std::string& room_id, const ProtocolMessage& message, const std::string& exclude_player) {
    if (!running.load()) {
        logger->warning("Cannot broadcast - server not running");
        return;
    }

    // Get all players in the room
    std::vector<std::string> room_players = playerManager->getPlayersInRoom(room_id);
    if (room_players.empty()) {
        logger->debug("No players to broadcast to in room " + room_id);
        return;
    }

    std::string broadcast_msg = message.serialize() + "\n";
    int successful_sends = 0;
    int failed_sends = 0;

    logger->debug("Broadcasting message type " + std::to_string(static_cast<int>(message.getType())) + " to room " + room_id);

    for (const std::string& player_name : room_players) {
        // Skip excluded player
        if (player_name == exclude_player) {
            continue;
        }

        // Get player info
        auto player_opt = playerManager->getPlayer(player_name);
        if (!player_opt.has_value() || !player_opt->connected || player_opt->socket_fd == -1) {
            logger->debug("Skipping broadcast to disconnected player: " + player_name);
            continue;
        }

        int socket_fd = player_opt->socket_fd;

        // Send message to this player
        ssize_t bytes_sent = send(socket_fd, broadcast_msg.c_str(), broadcast_msg.length(), MSG_NOSIGNAL);

        if (bytes_sent < 0) {
            logger->warning("Failed to broadcast to player '" + player_name + "' on socket " + std::to_string(socket_fd) + ": " + std::string(strerror(errno)));
            failed_sends++;
        } else if (static_cast<size_t>(bytes_sent) != broadcast_msg.length()) {
            logger->warning("Partial broadcast to player '" + player_name + "': sent " + std::to_string(bytes_sent) + "/" + std::to_string(broadcast_msg.length()) + " bytes");
            failed_sends++;
        } else {
            logger->debug("Broadcast sent to player '" + player_name + "' on socket " + std::to_string(socket_fd));
            successful_sends++;
        }
    }

    logger->info("Broadcast to room " + room_id + " complete: " + std::to_string(successful_sends) + " successful, " + std::to_string(failed_sends) + " failed");
}
