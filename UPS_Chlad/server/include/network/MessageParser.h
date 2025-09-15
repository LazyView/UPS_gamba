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
    static std::string getPlayerNameFromMessage(const ProtocolMessage& msg);
    static bool requiresActivePlayer(MessageType type);
    static std::string extractDataField(const ProtocolMessage& msg, const std::string& key);
};

#endif //MESSAGEPARSER_H