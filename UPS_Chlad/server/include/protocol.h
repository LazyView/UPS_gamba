// protocol.h - Basic Gamba Game Protocol
// KIV/UPS Network Programming Project

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string>
#include <map>
#include <sstream>
#include <vector>

// Message types for client-server communication
enum class MessageType {
    // Client -> Server
    CONNECT = 0,	// Client requesting connection
    DISCONNECT = 1,	// Client requesting disconnection
    JOIN_ROOM = 2,	// Client requesting room joining
    LEAVE_ROOM = 3,	// Clint requesting room leaving
    PING = 4,		// Client pinging server
	START_GAME = 5,	// Client requesting game start
	RECONNECT = 6,
    PLAY_CARDS = 7,         // Client plays cards
    PICKUP_PILE = 8,        // Client picks up discard pile

    // Server -> Client
    CONNECTED = 100, 			// Server notifies about connection
    ROOM_JOINED = 101,			// Sever notifies about Player joining room
    ROOM_LEFT = 102,			// Server notifies about Player leaving room
    ERROR_MSG = 103,			// Server notifies error message was sent
    PONG = 104,					// Server replies pong to client
	GAME_STARTED = 105,			// Server notifies game start
	GAME_STATE = 106,			// Server notifies game state
	PLAYER_DISCONNECTED = 107,
	GAME_PAUSED = 108,
	PLAYER_RECONNECTED = 109,
	GAME_RESUMED = 110,
    TURN_RESULT = 111,      // Server responds to turn actions
    GAME_OVER = 112         // Server announces game completion
};

// Simple protocol message structure
struct ProtocolMessage {
    MessageType type;
    std::string player_id;
    std::string room_id;
    std::map<std::string, std::string> data;

    // Default constructor
    ProtocolMessage() : type(MessageType::PING), player_id(""), room_id("") {}

    // Constructor with type
    ProtocolMessage(MessageType msg_type) : type(msg_type), player_id(""), room_id("") {}

    // Serialize message to string format: TYPE|PLAYER_ID|ROOM_ID|KEY1=VALUE1|KEY2=VALUE2
    std::string serialize() const {
        std::stringstream ss;
        ss << static_cast<int>(type) << "|" << player_id << "|" << room_id;

        for (const auto& pair : data) {
            ss << "|" << pair.first << "=" << pair.second;
        }

        return ss.str();
    }

    // Parse message from string format
    static ProtocolMessage parse(const std::string& message) {
        ProtocolMessage msg;
        std::istringstream iss(message);
        std::string token;

        // DEBUG: Show what we're parsing
        std::cout << "🔍 PARSING: '" << message << "'" << std::endl;

        try {
            // Parse message type
            if (std::getline(iss, token, '|')) {
                msg.type = static_cast<MessageType>(std::stoi(token));
                std::cout << "🔍   Type: " << token << std::endl;
            }

            // Parse player ID
            if (std::getline(iss, token, '|')) {
                msg.player_id = token;
                std::cout << "🔍   Player ID: '" << token << "'" << std::endl;
            }

            // Parse room ID
            if (std::getline(iss, token, '|')) {
                msg.room_id = token;
                std::cout << "🔍   Room ID: '" << token << "'" << std::endl;
            }

            // Parse key-value pairs
            int kvp_count = 0;
            while (std::getline(iss, token, '|')) {
                kvp_count++;
                std::cout << "🔍   KVP #" << kvp_count << ": '" << token << "'" << std::endl;

                size_t pos = token.find('=');
                if (pos != std::string::npos) {
                    std::string key = token.substr(0, pos);
                    std::string value = token.substr(pos + 1);
                    std::cout << "🔍     Key: '" << key << "', Value: '" << value << "'" << std::endl;
                    msg.data[key] = value;
                }
            }

            std::cout << "🔍   Total KVPs found: " << kvp_count << std::endl;

        } catch (const std::exception& e) {
            std::cout << "🔍 PARSE ERROR: " << e.what() << std::endl;
            msg.type = MessageType::ERROR_MSG;
            msg.data["error"] = "Invalid message format";
        }

        return msg;
    }

    // Helper methods
    void setData(const std::string& key, const std::string& value) {
        data[key] = value;
    }

    std::string getData(const std::string& key, const std::string& default_value = "") const {
        auto it = data.find(key);
        return (it != data.end()) ? it->second : default_value;
    }

    bool hasData(const std::string& key) const {
        return data.find(key) != data.end();
    }
};

// Protocol helper functions
class ProtocolHelper {
public:
    // Create common response messages
    static ProtocolMessage createConnectedResponse(const std::string& player_id, const std::string& player_name) {
        ProtocolMessage msg(MessageType::CONNECTED);
        msg.player_id = player_id;
        msg.setData("name", player_name);
        msg.setData("status", "success");
        return msg;
    }

    static ProtocolMessage createRoomJoinedResponse(const std::string& player_id, const std::string& room_id) {
        ProtocolMessage msg(MessageType::ROOM_JOINED);
        msg.player_id = player_id;
        msg.room_id = room_id;
        msg.setData("status", "success");
        return msg;
    }

    static ProtocolMessage createErrorResponse(const std::string& error_message) {
        ProtocolMessage msg(MessageType::ERROR_MSG);
        msg.setData("error", error_message);
        return msg;
    }

    static ProtocolMessage createPongResponse() {
        ProtocolMessage msg(MessageType::PONG);
        return msg;
    }

    // Validate message format
    static bool isValidMessage(const std::string& message) {
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

    // Get message type name for logging
    static std::string getMessageTypeName(MessageType type) {
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
};

#endif // PROTOCOL_H