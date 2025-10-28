//
// Created by chlad on 9/12/2025.
//

#ifndef MESSAGEVALIDATOR_H
#define MESSAGEVALIDATOR_H

#include "ProtocolMessage.h"
#include <string>

class MessageValidator {
public:
    /*
    * Validates if message has valid format. Looks for first | and validates if first part of message is message type from 0 to 200
    * @param raw_message - message as it was received
    * @return true, if message is valid, else false
    */
    bool isValidFormat(const std::string& raw_message);
    /*
    * Validates if message has valid type (existing)
    * @param raw_message - message as it was received
    * @return true if the type is valid, else false
    */
    bool isValidMessageType(int type_code);
    /*
    * Validates if each message type contains required data
    * @return true, if message contains required data, false if not or is unknown message
    */
    bool isValidMessage(const ProtocolMessage& msg);  // Add this method
};

#endif //MESSAGEVALIDATOR_H