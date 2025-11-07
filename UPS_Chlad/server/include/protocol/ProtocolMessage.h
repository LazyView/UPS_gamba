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
    bool should_broadcast_to_room = false;

    // Field code mappings (old verbose names → new compact codes)
    static const std::map<std::string, std::string> FIELD_CODE_MAP;
    static const std::map<std::string, std::string> REVERSE_FIELD_CODE_MAP;

    // Helper to get compact code for a field name
    static std::string getCompactCode(const std::string& field_name);
    static std::string getFullFieldName(const std::string& compact_code);

    // Constructors
    ProtocolMessage();
    ProtocolMessage(MessageType msg_type);

    /**
    * Serialize message to string format: TYPE|PLAYER_ID|ROOM_ID|KEY1=VALUE1|KEY2=VALUE2
    */
    std::string serialize() const;
    /*
    * Parses string message in this format type|player|room|key_value1|key_value2|..|..|key_valuen
    * @return ProtocolMessage format
    */
    static ProtocolMessage parse(const std::string& message);

    // Helper methods
    /*
    * Sets message key values
    */
    void setData(const std::string& key, const std::string& value);
    /*
    * Retrieves message key value
    * @return key value as string
    */
    std::string getData(const std::string& key, const std::string& default_value = "") const;
    /*
    * Checks if message has data on position of key.
    * @return true if not empty, else false
    */
    bool hasData(const std::string& key) const;

    // Convenience methods
    /*
    * @return message type (eg. connect(0), reconnect(6), join_room(2)..)
    */
    MessageType getType() const { return type; }
    /*
    * Sets player id
    * @param string id - player id
    */
    void setPlayerId(const std::string& id) { player_id = id; }
    /*
    * Sets room id
    * @param string id - room id
    */
    void setRoomId(const std::string& id) { room_id = id; }
    /*
    * @return players id (name)
    */
    std::string getPlayerId() const { return player_id; }
    /*
    * @return room id (name)
    */
    std::string getRoomId() const { return room_id; }
};

#endif // PROTOCOL_MESSAGE_H