//
// Created by chlad on 9/12/2025.
//

#include "MessageValidator.h"
#include "MessageType.h"
#include <string>
#include <stdexcept>

bool MessageValidator::isValidFormat(const std::string& raw_message) {
    if (raw_message.empty()) {
        return false;
    }

    size_t first_pipe = raw_message.find('|');
    if (first_pipe == std::string::npos) {
        return false;
    }

    std::string type_str = raw_message.substr(0, first_pipe);

    try {
        int type_code = std::stoi(type_str);
        // Check reasonable range (0-200 is safe for your protocol)
        if (type_code < 0 || type_code > 200) {
            return false;
        }
    } catch (const std::exception& e) {
        return false;  // Not a number
    }

    return true;
}

bool MessageValidator::isValidMessageType(int type_code) {
    // Check if type_code corresponds to a valid MessageType
    switch (static_cast<MessageType>(type_code)) {
        case MessageType::CONNECT:
        case MessageType::DISCONNECT:
        case MessageType::JOIN_ROOM:
        case MessageType::LEAVE_ROOM:
        case MessageType::PING:
        case MessageType::START_GAME:
        case MessageType::RECONNECT:
        case MessageType::PLAY_CARDS:
        case MessageType::PICKUP_PILE:
        case MessageType::CONNECTED:
        case MessageType::ROOM_JOINED:
        case MessageType::ROOM_LEFT:
        case MessageType::ERROR_MSG:
        case MessageType::PONG:
        case MessageType::GAME_STARTED:
        case MessageType::GAME_STATE:
        case MessageType::PLAYER_DISCONNECTED:
        case MessageType::GAME_PAUSED:
        case MessageType::PLAYER_RECONNECTED:
        case MessageType::GAME_RESUMED:
        case MessageType::TURN_RESULT:
        case MessageType::GAME_OVER:
            return true;
        default:
            return false;
    }
}

bool MessageValidator::isValidMessage(const ProtocolMessage& msg) {
    switch (msg.getType()) {
        case MessageType::CONNECT:
            return msg.hasData("name") && !msg.getData("name").empty();

        case MessageType::JOIN_ROOM:
            return true;  // room_id can be empty (auto-assign)

        case MessageType::PING:
            return true;  // No required fields

        case MessageType::START_GAME:
            return true;  // No required fields

        case MessageType::PLAY_CARDS:
            return msg.hasData("cards") && !msg.getData("cards").empty();

        case MessageType::PICKUP_PILE:
            return true;  // No required fields

        case MessageType::RECONNECT:
            return !msg.player_id.empty();  // Need player_id for reconnect

        default:
            return false;  // Unknown message type
    }
}