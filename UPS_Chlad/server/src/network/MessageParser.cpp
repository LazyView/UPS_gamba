//
// Created by chlad on 9/12/2025.
//

#include "MessageParser.h"

bool MessageParser::requiresActivePlayer(MessageType type){
    if(type == MessageType::RECONNECT || type == MessageType::CONNECT){
        return false;
    }
    return true;
}

std::string MessageParser::getPlayerNameFromMessage(const ProtocolMessage& msg) {
    switch (msg.getType()) {
        case MessageType::CONNECT:
        case MessageType::RECONNECT:
            return msg.getData("name");  // Both use the same format: name=<player>

        default:
            return "";
    }
}

std::string MessageParser::extractDataField(const ProtocolMessage& msg, const std::string& key) {
    if(msg.hasData(key)){
        return msg.getData(key);
    }
    return "";
}