//
// Created by chlad on 9/11/2025.
//

#include "MessageHandler.h"
#include "MessageParser.h"
#include "ProtocolHelper.h"
#include "MessageValidator.h"
#include "PlayerManager.h"
#include "RoomManager.h"
#include <sstream>
#include <vector>

MessageHandler::MessageHandler(PlayerManager* pm, RoomManager* rm, MessageValidator* mv)
    : playerManager(pm), roomManager(rm), validator(mv) {}

ProtocolMessage MessageHandler::processMessage(const std::string& raw_message, int client_socket) {
    if (!validator->isValidFormat(raw_message)) {
        ProtocolMessage disconnect_response(MessageType::ERROR_MSG);
        disconnect_response.setData("disconnect", "true");
        return disconnect_response;
    }

    ProtocolMessage msg = ProtocolMessage::parse(raw_message);

    if (!validator->isValidMessageType(static_cast<int>(msg.getType()))) {
        ProtocolMessage disconnect_response(MessageType::ERROR_MSG);
        disconnect_response.setData("disconnect", "true");
        return disconnect_response;
    }

    // A. Player resolution
    std::string player_name = "";

    if (MessageParser::requiresActivePlayer(msg.getType())) {
        // Most messages need active player from socket
        player_name = playerManager->getPlayerIdFromSocket(client_socket);
        if (player_name.empty()) {
            return ProtocolHelper::createErrorResponse("Must connect first");
        }
    }

    switch (msg.getType()) {
        case MessageType::CONNECT:
            return handleConnect(msg, client_socket);
        case MessageType::RECONNECT:
            return handleReconnect(msg, client_socket);
        case MessageType::PING:
            return handlePing(player_name);
        case MessageType::JOIN_ROOM:
            return handleJoinRoom(player_name);
        case MessageType::LEAVE_ROOM:
            return handleLeaveRoom(player_name);
        case MessageType::START_GAME:
            return handleStartGame(player_name);
        case MessageType::PLAY_CARDS:
            return handlePlayCards(msg, player_name);
        case MessageType::PICKUP_PILE:
            return handlePickupPile(player_name);
        default:
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
    if(player_name.empty()) {
        return ProtocolHelper::createErrorResponse("Invalid name");
    }
    std::string result = playerManager->connectPlayer(player_name, client_socket);

    // 4. Return response
    if (!result.empty()) {
        // Success - result contains the player name
        return ProtocolHelper::createConnectedResponse(result, player_name);
    } else {
        // Failed - player name already taken or other error
        return ProtocolHelper::createErrorResponse("Connection failed - name already taken");
    }
}

ProtocolMessage MessageHandler::handleJoinRoom(const std::string& player_name) {
    // Try to join any available room
    std::string assigned_room = roomManager->joinAnyAvailableRoom(player_name);
    // 4. Return response
    if (!assigned_room.empty()) {
        // Success - result contains the player name
        return ProtocolHelper::createRoomJoinedResponse(player_name, assigned_room);
    } else {
        // Failed - player name already taken or other error
        return ProtocolHelper::createErrorResponse("Error occurred while joining room");
    }
}

ProtocolMessage MessageHandler::handleReconnect(const ProtocolMessage& msg, int client_socket) {
    std::string player_name = MessageParser::getPlayerNameFromMessage(msg);

    if (player_name.empty()) {
        return ProtocolHelper::createErrorResponse("Player name required");
    }
    std::string result = playerManager->connectPlayer(player_name, client_socket);

    if (result.empty()) {
        return ProtocolHelper::createErrorResponse("Reconnection failed");
    } else {
        return ProtocolHelper::createConnectedResponse(result, player_name);
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