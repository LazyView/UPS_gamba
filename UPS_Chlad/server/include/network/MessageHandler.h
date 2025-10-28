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
    
    // Main entry point
    /*
    * Main entry point once message is received, takes care of the parsing. Does routing to proper handler once message is parsed.
    * Handles invalid messages (kick player).
    * @return vector of response messages (used when broadcasting is required).
    */
    std::vector<ProtocolMessage> processMessage(const std::string& raw_message, int client_socket);

    /*
    * Handles request for connection with validation for lenght, invalid characters and returns response with result
    * @param msg sent by user
    * @param client_socket from where was message sent
    * @return vector of response messages
    */
    std::vector<ProtocolMessage> handleConnect(const ProtocolMessage& msg, int client_socket);
    /*
    * Handles request for reconnection. Checks players list to verify if player was connected and is marked as disconnected.
    * @param msg sent by user
    * @param client_socket from where was message sent
    * @return vector of response messages
    */
    std::vector<ProtocolMessage> handleReconnect(const ProtocolMessage& msg, int client_socket);
    /*
    * Handles request for connection with validation for lenght, invalid characters and returns response with result
    * @param msg sent by user
    * @param client_socket from where was message sent
    * @return vector of response messages
    */
    std::vector<ProtocolMessage> handlePlayCards(const ProtocolMessage& msg, const std::string& player_name);
    /*
    * Handles request for connection with validation for lenght, invalid characters and returns response with result
    * @param msg sent by user
    * @param client_socket from where was message sent
    * @return vector of response messages
    */
    std::vector<ProtocolMessage> handleJoinRoom(const std::string& player_name);
    /*
    * Handles request for connection with validation for lenght, invalid characters and returns response with result
    * @param msg sent by user
    * @param client_socket from where was message sent
    * @return vector of response messages
    */
    std::vector<ProtocolMessage> handleLeaveRoom(const std::string& player_name);
    /*
    * Handles request for connection with validation for lenght, invalid characters and returns response with result
    * @param msg sent by user
    * @param client_socket from where was message sent
    * @return vector of response messages
    */
    std::vector<ProtocolMessage> handlePing(const std::string& player_name);
    /*
    * Handles request for connection with validation for lenght, invalid characters and returns response with result
    * @param msg sent by user
    * @param client_socket from where was message sent
    * @return vector of response messages
    */
    std::vector<ProtocolMessage> handleStartGame(const std::string& player_name);
    /*
    * Handles request for connection with validation for lenght, invalid characters and returns response with result
    * @param msg sent by user
    * @param client_socket from where was message sent
    * @return vector of response messages
    */
    std::vector<ProtocolMessage> handlePickupPile(const std::string& player_name);
};

#endif //MESSAGEHANDLER_H
