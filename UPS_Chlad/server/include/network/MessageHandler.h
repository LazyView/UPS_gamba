//
// Created by chlad on 9/11/2025.
//

#ifndef MESSAGEHANDLER_H
#define MESSAGEHANDLER_H

#include "ProtocolMessage.h"
#include "ProtocolHelper.h"
#include <vector>

// Forward declarations
class PlayerManager;
class RoomManager;
class GameManager;
class MessageValidator;
class Logger;

class MessageHandler {
private:
    PlayerManager* playerManager;
    RoomManager* roomManager;
    GameManager* gameManager;
    MessageValidator* validator;
    Logger* logger;

public:
    MessageHandler(PlayerManager* pm, RoomManager* rm, MessageValidator* mv, Logger* lg, GameManager* gm);
    
    // Main entry point - NOW RETURNS VECTOR
    std::vector<ProtocolMessage> processMessage(const std::string& raw_message, int client_socket);

    // Individual message handlers - ALL RETURN VECTORS NOW
    std::vector<ProtocolMessage> handleConnect(const ProtocolMessage& msg, int client_socket);
    std::vector<ProtocolMessage> handleReconnect(const ProtocolMessage& msg, int client_socket);
    std::vector<ProtocolMessage> handlePlayCards(const ProtocolMessage& msg, const std::string& player_name);
    std::vector<ProtocolMessage> handleJoinRoom(const std::string& player_name);
    std::vector<ProtocolMessage> handleLeaveRoom(const std::string& player_name);
    std::vector<ProtocolMessage> handlePing(const std::string& player_name);
    std::vector<ProtocolMessage> handleStartGame(const std::string& player_name);
    std::vector<ProtocolMessage> handlePickupPile(const std::string& player_name);
};

#endif //MESSAGEHANDLER_H
