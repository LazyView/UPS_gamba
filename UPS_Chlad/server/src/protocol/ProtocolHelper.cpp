// ProtocolHelper.cpp - Protocol message creation utilities implementation
// KIV/UPS Network Programming Project

#include "../include/protocol/ProtocolHelper.h"
#include "../include/core/GameManager.h"  // For GameStateData

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

ProtocolMessage ProtocolHelper::createGameStateResponse(
    const std::string& player_name,
    const std::string& room_id,
    const GameStateData& game_data)
{
    ProtocolMessage msg(MessageType::GAME_STATE);
    msg.player_id = player_name;
    msg.room_id = room_id;
    
    // Build comma-separated list of player's hand cards
    std::string hand_str;
    for (size_t i = 0; i < game_data.hand_cards.size(); ++i) {
        if (i > 0) hand_str += ",";
        hand_str += game_data.hand_cards[i];
    }
    
    msg.setData("hand", hand_str);
    msg.setData("reserves", std::to_string(game_data.reserve_count));
    msg.setData("current_player", game_data.current_player);
    msg.setData("top_card", game_data.top_discard_card);
    msg.setData("must_play_low", game_data.must_play_seven_or_lower ? "true" : "false");
    msg.setData("your_turn", (player_name == game_data.current_player) ? "true" : "false");
    
    // NEW: Add deck and discard pile sizes
    msg.setData("deck_size", std::to_string(game_data.deck_size));
    msg.setData("discard_pile_size", std::to_string(game_data.discard_pile_size));
    
    // Parse other players info (format: "playername:handsize:reservesize")
    for (const std::string& player_info : game_data.other_players_info) {
        size_t first_colon = player_info.find(':');
        size_t second_colon = player_info.find(':', first_colon + 1);
        
        if (first_colon != std::string::npos && second_colon != std::string::npos) {
            std::string opponent_name = player_info.substr(0, first_colon);
            std::string hand_size = player_info.substr(first_colon + 1, second_colon - first_colon - 1);
            std::string reserve_size = player_info.substr(second_colon + 1);
            
            msg.setData("opponent_hand", hand_size);
            msg.setData("opponent_reserves", reserve_size);
            msg.setData("opponent_name", opponent_name);
        }
    }
    
    return msg;
}

ProtocolMessage ProtocolHelper::createGameOverResponse(const std::string& winner) {
    ProtocolMessage msg(MessageType::GAME_OVER);
    msg.setData("winner", winner);
    msg.setData("status", "game_over");
    return msg;
}

ProtocolMessage ProtocolHelper::createTurnUpdateResponse(
    const std::string& player_name,
    const std::string& /* room_id */,
    const GameStateData& game_data)
{
    ProtocolMessage msg(MessageType::TURN_UPDATE);
    msg.player_id = player_name;
    msg.room_id = "";  // Client already knows room (per design)

    // ALWAYS send: hand, top_card, discard_pile_size, your_turn
    // Build comma-separated list of player's hand cards
    std::string hand_str;
    for (size_t i = 0; i < game_data.hand_cards.size(); ++i) {
        if (i > 0) hand_str += ",";
        hand_str += game_data.hand_cards[i];
    }
    msg.setData("hand", hand_str);

    msg.setData("top_card", game_data.top_discard_card);
    msg.setData("discard_pile_size", std::to_string(game_data.discard_pile_size));
    msg.setData("your_turn", (player_name == game_data.current_player) ? "1" : "0");

    // CONDITIONALLY send: reserves (only if changed from 3)
    if (game_data.reserve_count != 3) {
        msg.setData("reserves", std::to_string(game_data.reserve_count));
    }

    // CONDITIONALLY send: deck_size (only if cards were drawn this turn)
    // For now, always send it since we can't track "previous" state easily
    // TODO: Optimize by tracking previous deck size
    if (game_data.deck_size > 0) {
        msg.setData("deck_size", std::to_string(game_data.deck_size));
    }

    // CONDITIONALLY send: must_play_low (only if active)
    if (game_data.must_play_seven_or_lower) {
        msg.setData("must_play_low", "1");
    } else {
        // Send 0 to explicitly reset (important for clarity)
        msg.setData("must_play_low", "0");
    }

    // Parse opponent info (format: "playername:handsize:reservesize")
    for (const std::string& player_info : game_data.other_players_info) {
        size_t first_colon = player_info.find(':');
        size_t second_colon = player_info.find(':', first_colon + 1);

        if (first_colon != std::string::npos && second_colon != std::string::npos) {
            std::string hand_size = player_info.substr(first_colon + 1, second_colon - first_colon - 1);
            std::string reserve_size = player_info.substr(second_colon + 1);

            msg.setData("opponent_hand", hand_size);

            // Only send opponent reserves if not 3 (changed)
            if (reserve_size != "3") {
                msg.setData("opponent_reserves", reserve_size);
            }
        }
    }

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
        case MessageType::TURN_UPDATE: return "TURN_UPDATE";
        default: return "UNKNOWN";
    }
}
