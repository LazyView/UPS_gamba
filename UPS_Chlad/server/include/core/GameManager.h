#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H

#include <string>
#include <vector>

// Forward declarations
class Card;
class RoomManager;

// Data structure for game state (not ProtocolMessage)
struct GameStateData {
    std::vector<std::string> hand_cards;
    int reserve_count;
    std::string current_player;
    std::string top_discard_card;
    std::vector<std::string> other_players_info;  // Format: "playername:handsize:reservesize"
    bool must_play_seven_or_lower;
    bool valid;  // true if data is valid
    std::string error_message;  // if not valid, why?
    
    // NEW: Additional game state info
    int deck_size;              // Cards left in draw pile
    int discard_pile_size;      // Cards in discard pile

    // Constructor to initialize defaults
    GameStateData() : reserve_count(0), must_play_seven_or_lower(false), valid(false),
                      deck_size(0), discard_pile_size(0) {}
};

class GameManager {
public:
    GameManager();

    // Game state queries - returns raw data, not ProtocolMessage
    GameStateData getGameStateForPlayer(RoomManager* roomManager, const std::string& room_id, const std::string& player_name);

    // Game actions - takes RoomManager pointer and room_id
    bool playCards(RoomManager* roomManager, const std::string& room_id, const std::string& player_name, const std::vector<std::string>& card_strings);
    bool pickupPile(RoomManager* roomManager, const std::string& room_id, const std::string& player_name);
    bool startGame(RoomManager* roomManager, const std::string& room_id);

    // Game queries
    bool isGameActive(RoomManager* roomManager, const std::string& room_id);
    std::string getCurrentPlayer(RoomManager* roomManager, const std::string& room_id);
    
    // NEW: Game end detection
    bool isGameOver(RoomManager* roomManager, const std::string& room_id);
    std::string getWinner(RoomManager* roomManager, const std::string& room_id);

private:
    // Helper methods for protocol conversion
    Card parseCardFromString(const std::string& cardStr);
    std::vector<std::string> convertCardsToStrings(const std::vector<Card>& cards);
};

#endif // GAMEMANAGER_H