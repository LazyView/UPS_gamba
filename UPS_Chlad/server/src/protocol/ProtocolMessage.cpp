// ProtocolMessage.cpp - Protocol message structure implementation
// KIV/UPS Network Programming Project

#include "../include/protocol/ProtocolMessage.h"
#include <sstream>
#include <iostream>

// Constructors
ProtocolMessage::ProtocolMessage() : type(MessageType::PING), player_id(""), room_id("") {}

ProtocolMessage::ProtocolMessage(MessageType msg_type) : type(msg_type), player_id(""), room_id("") {}

// Serialize message to string format: TYPE|PLAYER_ID|ROOM_ID|KEY1=VALUE1|KEY2=VALUE2
std::string ProtocolMessage::serialize() const {
    std::stringstream ss;
    ss << static_cast<int>(type) << "|" << player_id << "|" << room_id;

    for (const auto& pair : data) {
        ss << "|" << pair.first << "=" << pair.second;
    }

    return ss.str();
}

// Parse message from string format
ProtocolMessage ProtocolMessage::parse(const std::string& message) {
    ProtocolMessage msg;
    std::istringstream iss(message);
    std::string token;

    // DEBUG: Show what we're parsing
    #ifdef DEBUG_PROTOCOL
    std::cout << "PARSING: '" << message << "'" << std::endl;
    #endif

    try {
        // Parse message type
        if (std::getline(iss, token, '|')) {
            msg.type = static_cast<MessageType>(std::stoi(token));
            #ifdef DEBUG_PROTOCOL
            std::cout << "   Type: " << token << std::endl;
            #endif
        }

        // Parse player ID
        if (std::getline(iss, token, '|')) {
            msg.player_id = token;
            #ifdef DEBUG_PROTOCOL
            std::cout << "   Player ID: '" << token << "'" << std::endl;
            #endif
        }

        // Parse room ID
        if (std::getline(iss, token, '|')) {
            msg.room_id = token;
            #ifdef DEBUG_PROTOCOL
            std::cout << "   Room ID: '" << token << "'" << std::endl;
            #endif
        }

        // Parse key-value pairs
        int kvp_count = 0;
        while (std::getline(iss, token, '|')) {
            kvp_count++;
            #ifdef DEBUG_PROTOCOL
            std::cout << "   KVP #" << kvp_count << ": '" << token << "'" << std::endl;
            #endif

            size_t pos = token.find('=');
            if (pos != std::string::npos) {
                std::string key = token.substr(0, pos);
                std::string value = token.substr(pos + 1);
                #ifdef DEBUG_PROTOCOL
                std::cout << "     Key: '" << key << "', Value: '" << value << "'" << std::endl;
                #endif
                msg.data[key] = value;
            }
        }

        #ifdef DEBUG_PROTOCOL
        std::cout << "   Total KVPs found: " << kvp_count << std::endl;
        #endif

    } catch (const std::exception& e) {
        #ifdef DEBUG_PROTOCOL
        std::cout << "PARSE ERROR: " << e.what() << std::endl;
        #endif
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