// ProtocolHelper.cpp - Protocol message creation utilities implementation
// KIV/UPS Network Programming Project

#include "../include/protocol/ProtocolHelper.h"

ProtocolMessage ProtocolHelper::createConnectedResponse(const std::string& player_id, const std::string& player_name) {
    ProtocolMessage msg(MessageType::CONNECTED);
    msg.player_id = player_id;
    msg.setData("name", player_name);
    msg.setData("status", "success");
    return msg;
}

ProtocolMessage ProtocolHelper::createRoomJoinedResponse(const std::string& player_id, const std::string& room_id) {
    ProtocolMessage msg(MessageType::ROOM_JOINED);
    msg.player_id = player_id;
    msg.room_id = room_id;
    msg.setData("status", "success");
    return msg;
}

ProtocolMessage ProtocolHelper::createRoomLeftResponse(const std::string& player_id) {
    ProtocolMessage msg(MessageType::ROOM_LEFT);
    msg.player_id = player_id;
    msg.room_id = "";  // No longer in any room
    msg.setData("status", "left");
    return msg;
}

ProtocolMessage ProtocolHelper::createGameStartedResponse() {
    ProtocolMessage msg(MessageType::GAME_STARTED);
    msg.setData("status", "started");
    return msg;
}

ProtocolMessage ProtocolHelper::createTurnResultResponse(const std::string& result) {
    ProtocolMessage msg(MessageType::TURN_RESULT);
    msg.setData("result", result);
    msg.setData("status", "success");
    return msg;
}

ProtocolMessage ProtocolHelper::createErrorResponse(const std::string& error_message) {
    ProtocolMessage msg(MessageType::ERROR_MSG);
    msg.setData("error", error_message);
    return msg;
}

ProtocolMessage ProtocolHelper::createPongResponse() {
    ProtocolMessage msg(MessageType::PONG);
    return msg;
}

bool ProtocolHelper::isValidMessage(const std::string& message) {
    // Basic validation - check if message has at least type field
    if (message.empty()) return false;

    size_t first_pipe = message.find('|');
    if (first_pipe == std::string::npos) return false;

    try {
        std::string type_str = message.substr(0, first_pipe);
        int type_int = std::stoi(type_str);
        return (type_int >= 0 && type_int <= 200); // Reasonable range for message types
    } catch (...) {
        return false;
    }
}

ProtocolMessage createGameStateResponse(const std::string& player_id,
                                                       const std::vector<std::string>& hand_cards,
                                                       int reserve_count,
                                                       const std::string& current_player,
                                                       const std::string& top_discard_card,
                                                       const std::vector<std::string>& other_players_info,
                                                       bool must_play_seven_or_lower) {
    ProtocolMessage msg(MessageType::GAME_STATE);
    msg.player_id = player_id;

    // Convert hand cards to comma-separated string
    std::string hand_str = "";
    for (size_t i = 0; i < hand_cards.size(); ++i) {
        if (i > 0) hand_str += ",";
        hand_str += hand_cards[i];
    }

    // Convert other players info to comma-separated string
    std::string other_players_str = "";
    for (size_t i = 0; i < other_players_info.size(); ++i) {
        if (i > 0) other_players_str += ",";
        other_players_str += other_players_info[i];
    }

    msg.setData("hand_cards", hand_str);
    msg.setData("reserve_count", std::to_string(reserve_count));
    msg.setData("current_player", current_player);
    msg.setData("top_discard", top_discard_card);
    msg.setData("other_players", other_players_str);
    msg.setData("must_play_seven_or_lower", must_play_seven_or_lower ? "true" : "false");
    msg.setData("status", "game_state");

    return msg;
}

std::string ProtocolHelper::getMessageTypeName(MessageType type) {
    switch (type) {
        case MessageType::CONNECT: return "CONNECT";
        case MessageType::DISCONNECT: return "DISCONNECT";
        case MessageType::JOIN_ROOM: return "JOIN_ROOM";
        case MessageType::LEAVE_ROOM: return "LEAVE_ROOM";
        case MessageType::PING: return "PING";
        case MessageType::CONNECTED: return "CONNECTED";
        case MessageType::ROOM_JOINED: return "ROOM_JOINED";
        case MessageType::ROOM_LEFT: return "ROOM_LEFT";
        case MessageType::ERROR_MSG: return "ERROR";
        case MessageType::PONG: return "PONG";
        case MessageType::START_GAME: return "START_GAME";
        case MessageType::GAME_STARTED: return "GAME_STARTED";
        case MessageType::GAME_STATE: return "GAME_STATE";
        case MessageType::RECONNECT: return "RECONNECT";
        case MessageType::PLAY_CARDS: return "PLAY_CARDS";
        case MessageType::PICKUP_PILE: return "PICKUP_PILE";
        case MessageType::PLAYER_DISCONNECTED: return "PLAYER_DISCONNECTED";
        case MessageType::GAME_PAUSED: return "GAME_PAUSED";
        case MessageType::PLAYER_RECONNECTED: return "PLAYER_RECONNECTED";
        case MessageType::GAME_RESUMED: return "GAME_RESUMED";
        case MessageType::TURN_RESULT: return "TURN_RESULT";
        case MessageType::GAME_OVER: return "GAME_OVER";
        default: return "UNKNOWN";
    }
}