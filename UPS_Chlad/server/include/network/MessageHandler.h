//
// Created by chlad on 9/11/2025.
//

#ifndef MESSAGEHANDLER_H
#define MESSAGEHANDLER_H

#include "ProtocolMessage.h"
#include "ProtocolHelper.h"

// Forward declarations
class PlayerManager;
class RoomManager;
class MessageValidator;
class Logger;

class MessageHandler {
private:
    PlayerManager* playerManager;
    RoomManager* roomManager;
    MessageValidator* validator;
    Logger* logger;

public:
    MessageHandler(PlayerManager* pm, RoomManager* rm, MessageValidator* mv, Logger* lg); // ADD Logger* lg
    // Main entry point
    ProtocolMessage processMessage(const std::string& raw_message, int client_socket);

    // Individual message handlers
    // Handlers that need message data
    ProtocolMessage handleConnect(const ProtocolMessage& msg, int client_socket);
    ProtocolMessage handleReconnect(const ProtocolMessage& msg, int client_socket);
    ProtocolMessage handlePlayCards(const ProtocolMessage& msg, const std::string& player_name);

    // Handlers that only need player name
    ProtocolMessage handleJoinRoom(const std::string& player_name);
    ProtocolMessage handleLeaveRoom(const std::string& player_name);
    ProtocolMessage handlePing(const std::string& player_name);  // Fixed: removed 'roo'
    ProtocolMessage handleStartGame(const std::string& player_name);
    ProtocolMessage handlePickupPile(const std::string& player_name);
};

#endif //MESSAGEHANDLER_H