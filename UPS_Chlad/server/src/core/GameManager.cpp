#include "GameManager.h"
#include "RoomManager.h"
#include "Room.h"
#include "../game/CardDeck.h"
#include "../game/GameLogic.h"

GameManager::GameManager() {
}

bool GameManager::playCards(RoomManager* roomManager, const std::string& room_id, 
                           const std::string& player_name, const std::vector<std::string>& card_strings) {
    
    return roomManager->withRoom(room_id, [&](Room* room) -> bool {
        if (!room || !room->isGameActive()) return false;
        
        // Convert protocol strings to domain objects
        std::vector<Card> cardObjects;
        for (const std::string& cardStr : card_strings) {
            Card card = parseCardFromString(cardStr);
            if (card.rank == Rank::ACE && card.suit == Suit::HEARTS && cardStr != "AH") {
                return false; // Invalid card string
            }
            cardObjects.push_back(card);
        }
        
        // Execute game logic safely within lock
        return room->gameLogic->playCards(player_name, cardObjects);
    });
}

bool GameManager::pickupPile(RoomManager* roomManager, const std::string& room_id, 
                            const std::string& player_name) {
    
    return roomManager->withRoom(room_id, [&](Room* room) -> bool {
        if (!room || !room->isGameActive()) return false;
        
        return room->gameLogic->pickupDiscardPile(player_name);
    });
}

bool GameManager::startGame(RoomManager* roomManager, const std::string& room_id) {
    return roomManager->withRoom(room_id, [&](Room* room) -> bool {
        if (!room){
            return false;
        }

        if (room->players.size() < 2){
            return false; // Need at least 2 players
        }

        return room->startGame();
    });
}

GameStateData GameManager::getGameStateForPlayer(RoomManager* roomManager, const std::string& room_id, 
                                                 const std::string& player_name) {
    
    return roomManager->withRoom(room_id, [&](Room* room) -> GameStateData {
        GameStateData result;
        result.valid = false;
        
        if (!room) {
            result.error_message = "Room not found";
            return result;
        }
        
        if (!room->isGameActive()) {
            result.error_message = "Game not active";
            return result;
        }
        
        // Get player's hand cards
        std::vector<Card> hand_cards = room->gameLogic->getPlayerHand(player_name);
        result.hand_cards = convertCardsToStrings(hand_cards);
        
        // Get reserve count (not actual cards)
        result.reserve_count = room->gameLogic->getPlayerReserveSize(player_name);
        
        // Get current player
        result.current_player = room->gameLogic->getCurrentPlayer();
        
        // Get top discard card
        if (!room->gameLogic->getDiscardPile().empty()) {
            result.top_discard_card = room->gameLogic->getTopDiscardCard().toString();
        }
        
        // Get other players' info
        for (const std::string& other_player : room->players) {
            if (other_player != player_name) {
                int hand_size = room->gameLogic->getPlayerHandSize(other_player);
                int reserve_size = room->gameLogic->getPlayerReserveSize(other_player);
                result.other_players_info.push_back(other_player + ":" + 
                                                   std::to_string(hand_size) + ":" + 
                                                   std::to_string(reserve_size));
            }
        }
        
        // Get special game state
        result.must_play_seven_or_lower = room->gameLogic->getMustPlaySevenOrLower();
        result.valid = true;
        
        return result;
    });
}

bool GameManager::isGameActive(RoomManager* roomManager, const std::string& room_id) {
    return roomManager->withRoom(room_id, [](Room* room) -> bool {
        return room && room->isGameActive();
    });
}

std::string GameManager::getCurrentPlayer(RoomManager* roomManager, const std::string& room_id) {
    return roomManager->withRoom(room_id, [](Room* room) -> std::string {
        if (!room || !room->isGameActive()) return "";
        return room->gameLogic->getCurrentPlayer();
    });
}

// Helper methods implementation
Card GameManager::parseCardFromString(const std::string& cardStr) {
    // Move the implementation from RoomManager here
    // ... (same implementation as before)
}

std::vector<std::string> GameManager::convertCardsToStrings(const std::vector<Card>& cards) {
    std::vector<std::string> result;
    for (const Card& card : cards) {
        result.push_back(card.toString());
    }
    return result;
}