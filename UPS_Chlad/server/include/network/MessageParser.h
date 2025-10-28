//
// Created by chlad on 9/12/2025.
//

#ifndef MESSAGEPARSER_H
#define MESSAGEPARSER_H

#include "ProtocolMessage.h"
#include "MessageType.h"
#include <string>

class MessageParser {
public:
    /*
    * Parses user name from message, used only for connect and reconnect. In other message types user is acquired by socket
    * @param message to be parsed
    * @return player name
    */
    static std::string getPlayerNameFromMessage(const ProtocolMessage& msg);
    /*
    * Checks if request requires connected player.
    * @param MessageType to be evaluated
    * @return true if requires, else false
    */
    static bool requiresActivePlayer(MessageType type);
    /*
    * Extracts one data field.
    * @param msg - message to extract from
    * @param key - data to be extracted
    * @return string if the extracted value
    */
    static std::string extractDataField(const ProtocolMessage& msg, const std::string& key);
};

#endif //MESSAGEPARSER_H