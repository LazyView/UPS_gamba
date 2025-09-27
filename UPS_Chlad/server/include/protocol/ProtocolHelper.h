//
// Created by chlad on 9/15/2025.
//

#ifndef PROTOCOLHELPER_H
#define PROTOCOLHELPER_H
#include <string>
#include <vector>
#include "MessageType.h"
#include "ProtocolMessage.h"

class ProtocolHelper {
public:
    // Create common response messages
    static ProtocolMessage createConnectedResponse(const std::string& player_id, const std::string& player_name);
    static ProtocolMessage createRoomJoinedResponse(const std::string& player_id, const std::string& room_id);
    static ProtocolMessage createRoomLeftResponse(const std::string& player_id);
    static ProtocolMessage createGameStartedResponse();
    static ProtocolMessage createTurnResultResponse(const std::string& result);
    static ProtocolMessage createErrorResponse(const std::string& error_message);
    static ProtocolMessage createPongResponse();
    static ProtocolMessage createGameStateResponse(const std::string& player_id,
                                                       const std::vector<std::string>& hand_cards,
                                                       int reserve_count,
                                                       const std::string& current_player,
                                                       const std::string& top_discard_card,
                                                       const std::map<std::string, std::pair<int,int>>& other_players_counts, // hand_size, reserve_count
                                                       bool must_play_seven_or_lower);
    // Validation utilities
    static bool isValidMessage(const std::string& message);

    // Logging utilities
    static std::string getMessageTypeName(MessageType type);
};
#endif //PROTOCOLHELPER_H