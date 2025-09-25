//
// Created by chlad on 9/11/2025.
//

#include "MessageHandler.h"
#include "MessageParser.h"
#include "ProtocolHelper.h"
#include "MessageValidator.h"
#include "PlayerManager.h"
#include "RoomManager.h"
#include "Logger.h"
#include <sstream>
#include <vector>

MessageHandler::MessageHandler(PlayerManager* pm, RoomManager* rm, MessageValidator* mv, Logger* lg)
    : playerManager(pm), roomManager(rm), validator(mv), logger(lg) {}

ProtocolMessage MessageHandler::processMessage(const std::string& raw_message, int client_socket) {
    if (!validator->isValidFormat(raw_message)) {
        logger->warning("Invalid message format from socket " + std::to_string(client_socket) + ": '" + raw_message + "'");
        ProtocolMessage disconnect_response(MessageType::ERROR_MSG);
        disconnect_response.setData("disconnect", "true");
        return disconnect_response;
    }

    ProtocolMessage msg = ProtocolMessage::parse(raw_message);
    logger->debug("Parsed message type: " + std::to_string(static_cast<int>(msg.getType()))); // ADD THIS
    if (!validator->isValidMessageType(static_cast<int>(msg.getType()))) {
        logger->warning("Invalid message type " + std::to_string(static_cast<int>(msg.getType())) + " from socket " + std::to_string(client_socket));
        ProtocolMessage disconnect_response(MessageType::ERROR_MSG);
        disconnect_response.setData("disconnect", "true");
        return disconnect_response;
    }
    logger->debug("Parsed message type: " + std::to_string(static_cast<int>(msg.getType())));

    // A. Player resolution
    std::string player_name = "";
    logger->debug("About to check if message requires active player"); // ADD THIS

    if (MessageParser::requiresActivePlayer(msg.getType())) {
        logger->debug("Message requires active player, getting from socket"); // ADD THIS

        // Most messages need active player from socket
        player_name = playerManager->getPlayerIdFromSocket(client_socket);
        logger->debug("Got player name from socket: '" + player_name + "'"); // ADD THIS

        if (player_name.empty()) {
            logger->debug("Player name is empty, returning error"); // ADD THIS

            return ProtocolHelper::createErrorResponse("Must connect first");
        }
    }
    logger->debug("About to enter switch statement with message type: " + std::to_string(static_cast<int>(msg.getType()))); // ADD THIS

    switch (msg.getType()) {
        case MessageType::CONNECT:
            logger->debug("Routing to handleConnect");
            return handleConnect(msg, client_socket);
        case MessageType::RECONNECT:
            logger->debug("Routing to handleReconnect");
            return handleReconnect(msg, client_socket);
        case MessageType::PING:
            logger->debug("Routing to handlePing");
            return handlePing(player_name);
        case MessageType::JOIN_ROOM:
            logger->debug("Routing to handleJoinRoom");
            return handleJoinRoom(player_name);
        case MessageType::LEAVE_ROOM:
            logger->debug("Routing to handleLeaveRoom");
            return handleLeaveRoom(player_name);
        case MessageType::START_GAME:
            logger->debug("Routing to handleStartGame");
            return handleStartGame(player_name);
        case MessageType::PLAY_CARDS:
            logger->debug("Routing to handlePlayCards");
            return handlePlayCards(msg, player_name);
        case MessageType::PICKUP_PILE:
            logger->debug("Routing to handlePickupPile");
            return handlePickupPile(player_name);
        default:
            logger->debug("Unknown message type");
            return ProtocolHelper::createErrorResponse("Unknown message type");
    }
}

ProtocolMessage MessageHandler::handlePing(const std::string& player_name) {
    playerManager->updateLastPing(player_name);
    return ProtocolHelper::createPongResponse();
}

ProtocolMessage MessageHandler::handleConnect(const ProtocolMessage& msg, int client_socket) {
    // 1. Get player name from message
    std::string player_name = MessageParser::getPlayerNameFromMessage(msg);
    logger->debug("handleConnect: extracted player name '" + player_name + "'");
    if(player_name.empty()) {
        logger->debug("handleConnect: player name is empty");
        return ProtocolHelper::createErrorResponse("Invalid name");
    }
    logger->debug("handleConnect: calling playerManager->connectPlayer with name='" + player_name + "', socket=" + std::to_string(client_socket)); // ADD THIS
    std::string result = playerManager->connectPlayer(player_name, client_socket);
    logger->debug("handleConnect: playerManager->connectPlayer returned '" + result + "'"); // ADD THIS

    // 4. Return response
    if (!result.empty()) {
        logger->debug("handleConnect: creating success response"); // ADD THIS
        // Success - result contains the player name
        return ProtocolHelper::createConnectedResponse(result, player_name);
    } else {
        logger->debug("handleConnect: creating error response"); // ADD THIS
        // Failed - player name already taken or other error
        return ProtocolHelper::createErrorResponse("Connection failed - name already taken");
    }
}

ProtocolMessage MessageHandler::handleJoinRoom(const std::string& player_name) {
    // Try to join any available room
    logger->debug("handleJoinRoom: called for player '" + player_name + "'"); // ADD THIS
    std::string assigned_room = roomManager->joinAnyAvailableRoom(player_name);
    logger->debug("handleJoinRoom: calling joinAnyAvailableRoom completed"); // ADD THIS
    // 4. Return response
    if (!assigned_room.empty()) {
        logger->debug("handleJoinRoom: room assigned successfully"); // ADD THIS
        // Success - result contains the player name
        return ProtocolHelper::createRoomJoinedResponse(player_name, assigned_room);
    } else {
        logger->debug("handleJoinRoom: room not assigned"); // ADD THIS
        // Failed - player name already taken or other error
        return ProtocolHelper::createErrorResponse("Error occurred while joining room");
    }
}

ProtocolMessage MessageHandler::handleReconnect(const ProtocolMessage& msg, int client_socket) {
    std::string player_name = MessageParser::getPlayerNameFromMessage(msg);

    if (player_name.empty()) {
        return ProtocolHelper::createErrorResponse("Player name required");
    }

    // Try to reconnect existing temporarily disconnected player
    if (playerManager->reconnectPlayer(player_name, client_socket)) {
        logger->info("Player '" + player_name + "' reconnected successfully");

        // TODO: Later we'll add game state synchronization here
        return ProtocolHelper::createConnectedResponse(player_name, player_name);
    } else {
        logger->warning("Reconnection failed for player '" + player_name + "' - not found or not disconnected");
        return ProtocolHelper::createErrorResponse("Reconnection failed - player not found or session expired");
    }
}

ProtocolMessage MessageHandler::handleLeaveRoom(const std::string& player_name) {
    bool result = roomManager->leaveRoom(player_name);

    if (result) {
        return ProtocolHelper::createRoomLeftResponse(player_name);
    } else {
        return ProtocolHelper::createErrorResponse("Leave room failed");
    }
}

ProtocolMessage MessageHandler::handleStartGame(const std::string& player_name) {
    std::string room_id = playerManager->getPlayerRoom(player_name);

    if (room_id.empty()) {
        return ProtocolHelper::createErrorResponse("Not in any room");
    }

    bool result = roomManager->startGame(room_id);

    if (result) {
        return ProtocolHelper::createGameStartedResponse();  // Simple response
    } else {
        return ProtocolHelper::createErrorResponse("Cannot start game");
    }
}

ProtocolMessage MessageHandler::handlePickupPile(const std::string& player_name) {
    bool result = roomManager->pickupPile(player_name);

    if (result) {
        return ProtocolHelper::createTurnResultResponse("pickup_success");
    } else {
        return ProtocolHelper::createErrorResponse("Cannot pickup pile");
    }
}

ProtocolMessage MessageHandler::handlePlayCards(const ProtocolMessage& msg, const std::string& player_name) {
    // 1. Extract cards from message
    std::string cards_str = MessageParser::extractDataField(msg, "cards");
    if (cards_str.empty()) {
        return ProtocolHelper::createErrorResponse("No cards specified");
    }

    // 2. Parse comma-separated cards
    std::vector<std::string> cards;
    std::stringstream ss(cards_str);
    std::string card;

    while (std::getline(ss, card, ',')) {
        cards.push_back(card);
    }

    // 3. Try to play cards
    bool result = roomManager->playCards(player_name, cards);

    if (result) {
        return ProtocolHelper::createTurnResultResponse("play_success");
    } else {
        return ProtocolHelper::createErrorResponse("Invalid card play");
    }
}