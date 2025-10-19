//
// Created by chlad on 9/12/2025.
//

#ifndef MESSAGEVALIDATOR_H
#define MESSAGEVALIDATOR_H

#include "ProtocolMessage.h"
#include <string>

class MessageValidator {
public:
    bool isValidFormat(const std::string& raw_message);
    bool isValidMessageType(int type_code);
    bool isValidMessage(const ProtocolMessage& msg);  // Add this method
};

#endif //MESSAGEVALIDATOR_H