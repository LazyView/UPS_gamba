// ProtocolMessage.cpp - Protocol message structure implementation
// KIV/UPS Network Programming Project

#include "../include/protocol/ProtocolMessage.h"
#include <sstream>
#include <iostream>

// Field code mappings: old verbose field names → new compact codes
const std::map<std::string, std::string> ProtocolMessage::FIELD_CODE_MAP = {
    // Field names
    {"hand", "h"},
    {"reserves", "r"},
    {"opponent_hand", "oh"},
    {"opponent_reserves", "or"},
    {"opponent_name", "on"},
    {"top_card", "tc"},
    {"discard_pile_size", "dp"},
    {"deck_size", "dk"},
    {"must_play_low", "ml"},
    {"your_turn", "yt"},
    {"current_player", "cp"},
    {"status", "st"},
    {"name", "nm"},
    {"error", "er"},
    {"result", "rs"},
    {"cards", "cd"},
    {"winner", "wn"},
    {"reconnected_player", "rp"},
    {"disconnected_player", "dc"},
    {"broadcast_type", "bt"},
    {"joined_player", "jp"},
    {"players", "pl"},
    {"player_count", "pc"},
    {"room_full", "rf"},
    {"disconnect", "disc"},
    {"message", "msg"},
    {"reason", "rsn"},

    // Status values
    {"temporarily_disconnected", "temp"},
    {"reconnected", "recon"},
    {"success", "ok"},
    {"game_over", "end"},
    {"started", "start"},
    {"left", "lft"},
    {"timed_out", "tout"},
    {"invalid_message", "inv"},

    // Result values
    {"play_success", "pok"},
    {"pickup_success", "uok"},
    {"opponent_disconnect", "opdc"},

    // Other common values
    {"room_notification", "rnotif"}
};

// Reverse mapping: compact codes → verbose field names (for debugging/logging)
const std::map<std::string, std::string> ProtocolMessage::REVERSE_FIELD_CODE_MAP = {
    // Field names
    {"h", "hand"},
    {"r", "reserves"},
    {"oh", "opponent_hand"},
    {"or", "opponent_reserves"},
    {"on", "opponent_name"},
    {"tc", "top_card"},
    {"dp", "discard_pile_size"},
    {"dk", "deck_size"},
    {"ml", "must_play_low"},
    {"yt", "your_turn"},
    {"cp", "current_player"},
    {"st", "status"},
    {"nm", "name"},
    {"er", "error"},
    {"rs", "result"},
    {"cd", "cards"},
    {"wn", "winner"},
    {"rp", "reconnected_player"},
    {"dc", "disconnected_player"},
    {"bt", "broadcast_type"},
    {"jp", "joined_player"},
    {"pl", "players"},
    {"pc", "player_count"},
    {"rf", "room_full"},
    {"disc", "disconnect"},
    {"msg", "message"},
    {"rsn", "reason"},

    // Status values
    {"temp", "temporarily_disconnected"},
    {"recon", "reconnected"},
    {"ok", "success"},
    {"end", "game_over"},
    {"start", "started"},
    {"lft", "left"},
    {"tout", "timed_out"},
    {"inv", "invalid_message"},

    // Result values
    {"pok", "play_success"},
    {"uok", "pickup_success"},
    {"opdc", "opponent_disconnect"},

    // Other common values
    {"rnotif", "room_notification"}
};

// Helper methods for field code translation
std::string ProtocolMessage::getCompactCode(const std::string& field_name) {
    auto it = FIELD_CODE_MAP.find(field_name);
    return (it != FIELD_CODE_MAP.end()) ? it->second : field_name;
}

std::string ProtocolMessage::getFullFieldName(const std::string& compact_code) {
    auto it = REVERSE_FIELD_CODE_MAP.find(compact_code);
    return (it != REVERSE_FIELD_CODE_MAP.end()) ? it->second : compact_code;
}

// Constructors
ProtocolMessage::ProtocolMessage() : type(MessageType::PING), player_id(""), room_id("") {}

ProtocolMessage::ProtocolMessage(MessageType msg_type) : type(msg_type), player_id(""), room_id("") {}

// Serialize message to string format: TYPE|PLAYER_ID|ROOM_ID|KEY1=VALUE1|KEY2=VALUE2
// Uses compact field codes for efficient transmission
std::string ProtocolMessage::serialize() const {
    std::stringstream ss;
    ss << static_cast<int>(type) << "|" << player_id << "|" << room_id;

    for (const auto& pair : data) {
        // Convert field name to compact code (or use as-is if no mapping exists)
        std::string compact_code = getCompactCode(pair.first);

        // ALSO convert field value if it's in the mapping (e.g., "temporarily_disconnected" → "temp")
        std::string compact_value = getCompactCode(pair.second);

        ss << "|" << compact_code << "=" << compact_value;
    }

    return ss.str();
}

// Parse message from string format
// Accepts compact field codes and converts to full field names for internal storage
ProtocolMessage ProtocolMessage::parse(const std::string& message) {
    ProtocolMessage msg;
    std::istringstream iss(message);
    std::string token;

    try {
        // Parse message type
        if (std::getline(iss, token, '|')) {
            msg.type = static_cast<MessageType>(std::stoi(token));
        }

        // Parse player ID
        if (std::getline(iss, token, '|')) {
            msg.player_id = token;
        }

        // Parse room ID
        if (std::getline(iss, token, '|')) {
            msg.room_id = token;
        }

        // Parse key-value pairs (may be compact codes or full names)
        int kvp_count = 0;
        while (std::getline(iss, token, '|')) {
            kvp_count++;

            size_t pos = token.find('=');
            if (pos != std::string::npos) {
                std::string key = token.substr(0, pos);
                std::string value = token.substr(pos + 1);

                // Convert compact code to full field name (or keep as-is if already full)
                std::string full_field_name = getFullFieldName(key);

                // ALSO convert compact value to full value (e.g., "temp" → "temporarily_disconnected")
                // BUT: Only if value is NOT a pure number (to avoid converting "1", "2", etc.)
                std::string full_value = value;
                if (!value.empty()) {
                    // Check if value is a number (digit or starts with - for negative)
                    bool is_numeric = true;
                    size_t start_idx = (value[0] == '-' && value.length() > 1) ? 1 : 0;
                    for (size_t i = start_idx; i < value.length(); ++i) {
                        if (!std::isdigit(value[i])) {
                            is_numeric = false;
                            break;
                        }
                    }

                    // Only convert if NOT numeric
                    if (!is_numeric) {
                        full_value = getFullFieldName(value);
                    }
                }

                msg.data[full_field_name] = full_value;
            }
        }
    } catch (const std::exception& e) {
        msg.type = MessageType::ERROR_MSG;
        msg.data["error"] = "Invalid message format";
    }

    return msg;
}

// Helper methods
void ProtocolMessage::setData(const std::string& key, const std::string& value) {
    data[key] = value;
}

std::string ProtocolMessage::getData(const std::string& key, const std::string& default_value) const {
    auto it = data.find(key);
    return (it != data.end()) ? it->second : default_value;
}

bool ProtocolMessage::hasData(const std::string& key) const {
    return data.find(key) != data.end();
}