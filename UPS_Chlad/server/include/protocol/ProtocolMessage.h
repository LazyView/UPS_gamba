// ProtocolMessage.h - Protocol message structure and utilities
// KIV/UPS Network Programming Project

#ifndef PROTOCOL_MESSAGE_H
#define PROTOCOL_MESSAGE_H

#include <string>
#include <map>
#include "MessageType.h"

struct ProtocolMessage {
    MessageType type;
    std::string player_id;
    std::string room_id;
    std::map<std::string, std::string> data;

    // Constructors
    ProtocolMessage();
    ProtocolMessage(MessageType msg_type);

    // Serialization
    std::string serialize() const;
    static ProtocolMessage parse(const std::string& message);

    // Helper methods
    void setData(const std::string& key, const std::string& value);
    std::string getData(const std::string& key, const std::string& default_value = "") const;
    bool hasData(const std::string& key) const;

    // Convenience methods
    MessageType getType() const { return type; }
    void setPlayerId(const std::string& id) { player_id = id; }
    void setRoomId(const std::string& id) { room_id = id; }
    std::string getPlayerId() const { return player_id; }
    std::string getRoomId() const { return room_id; }
};

#endif // PROTOCOL_MESSAGE_H