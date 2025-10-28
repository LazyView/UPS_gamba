//
// Created by chlad on 9/15/2025.
//

#ifndef PROTOCOLHELPER_H
#define PROTOCOLHELPER_H
#include <string>
#include <vector>
#include "MessageType.h"
#include "ProtocolMessage.h"

// Forward declaration
struct GameStateData;

class ProtocolHelper {
public:
    // Create common response messages
    /*
    * Takes params values as strings and builds ProtocolMessage response to connect request
    * @param player_id - players id
    * @param player_name - players name (same as id, this is just legacy code)
    * @return ProtocolMessage that will be sent to User after success
    */
    static ProtocolMessage createConnectedResponse(const std::string& player_id, const std::string& player_name);
    /*
    * Takes params values as strings and builds ProtocolMessage response to room join request
    * @param player_id - players id
    * @param room_id - rooms name
    * @return ProtocolMessage that will be sent to User after success
    */
    static ProtocolMessage createRoomJoinedResponse(const std::string& player_id, const std::string& room_id);
    /*
    * Takes params values as strings and builds ProtocolMessage response to room left request
    * @param player_id - players id
    * @return ProtocolMessage that will be sent to User after success
    */
    static ProtocolMessage createRoomLeftResponse(const std::string& player_id);
    /*
    * Builds ProtocolMessage response to start game request
    * @return ProtocolMessage that will be sent to all users once game starts
    */
    static ProtocolMessage createGameStartedResponse();
    /*
    * Takes params values as strings and builds ProtocolMessage response to play_card request
    * @param result - result of the round (valid or invalid play)
    * @return ProtocolMessage that will be sent to User after success
    */
    static ProtocolMessage createTurnResultResponse(const std::string& result);
    /*
    * Takes param value as string and builds ProtocolMessage response to wrong request
    * @param error_message - error message returned by server
    * @return ProtocolMessage that will be sent to User after success
    */
    static ProtocolMessage createErrorResponse(const std::string& error_message);
    /*
    * Builds ProtocolMessage response to ping request
    * @return ProtocolMessage that will be sent to User after success
    */
    static ProtocolMessage createPongResponse();
    
    /*
    * takes param values as strings and build ProtocolMessage response when game state is required.
    * @param player_name - player id (requester)
    * @param room_id - room id to send the request
    * @param game_data - has info about game (hand, reserve, current_player, top_card, must_play_low, your_turn, deck_size, discard_pile_size)
    * @return ProtocolMessage that contains comprehensive game data
    */
    static ProtocolMessage createGameStateResponse(
        const std::string& player_name,
        const std::string& room_id,
        const GameStateData& game_data
    );
    
    /*
    * Builds ProtocolMessage response for winning.
    * @return ProtocolMessage about the winner
    */
    static ProtocolMessage createGameOverResponse(const std::string& winner);
    
    // Validation utilities
    /*
    * Utility function to validate if message has atleast message type and lenght of the message type shorter then 200
    */
    static bool isValidMessage(const std::string& message);

    // Logging utilities
    /*
    * Function for logging, converts message type to string format
    */
    static std::string getMessageTypeName(MessageType type);
};
#endif //PROTOCOLHELPER_H
