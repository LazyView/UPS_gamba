#include "GameManager.h"
#include "RoomManager.h"
#include "Room.h"
#include "../game/CardDeck.h"
#include "../game/GameLogic.h"
#include <stdexcept>

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
        } else {
            // Empty pile - use placeholder card (1S)
            result.top_discard_card = "1S";  // Placeholder: any card can be played
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
        
        // NEW: Get deck and discard pile sizes
        result.deck_size = room->gameLogic->getDeckSize();
        result.discard_pile_size = room->gameLogic->getDiscardPile().size();
        
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

bool GameManager::isGameOver(RoomManager* roomManager, const std::string& room_id) {
    return roomManager->withRoom(room_id, [](Room* room) -> bool {
        if (!room || !room->isGameActive()) return false;
        
        // Check if any player has won (no cards left)
        for (const std::string& player : room->players) {
            int hand_size = room->gameLogic->getPlayerHandSize(player);
            int reserve_size = room->gameLogic->getPlayerReserveSize(player);
            
            if (hand_size == 0 && reserve_size == 0) {
                return true;  // Game over, this player won
            }
        }
        
        return false;
    });
}

std::string GameManager::getWinner(RoomManager* roomManager, const std::string& room_id) {
    return roomManager->withRoom(room_id, [](Room* room) -> std::string {
        if (!room || !room->isGameActive()) return "";
        
        // Find player with no cards left
        for (const std::string& player : room->players) {
            int hand_size = room->gameLogic->getPlayerHandSize(player);
            int reserve_size = room->gameLogic->getPlayerReserveSize(player);
            
            if (hand_size == 0 && reserve_size == 0) {
                return player;  // This player won
            }
        }
        
        return "";  // No winner yet
    });
}

// Helper methods implementation
Card GameManager::parseCardFromString(const std::string& cardStr) {
    if (cardStr.length() < 2) {
        throw std::invalid_argument("Invalid card string: " + cardStr);
    }

    // Parse suit (last character)
    char suitChar = cardStr[cardStr.length() - 1];
    Suit suit;
    switch (suitChar) {
        case 'H': suit = Suit::HEARTS; break;
        case 'D': suit = Suit::DIAMONDS; break;
        case 'C': suit = Suit::CLUBS; break;
        case 'S': suit = Suit::SPADES; break;
        default: throw std::invalid_argument("Invalid suit: " + std::string(1, suitChar));
    }

    // Parse rank (everything except last character)
    std::string rankStr = cardStr.substr(0, cardStr.length() - 1);
    Rank rank;
    
    if (rankStr == "A") {
        rank = Rank::ACE;
    } else if (rankStr == "J") {
        rank = Rank::JACK;
    } else if (rankStr == "Q") {
        rank = Rank::QUEEN;
    } else if (rankStr == "K") {
        rank = Rank::KING;
    } else {
        // Numeric rank (2-10)
        try {
            int rankValue = std::stoi(rankStr);
            if (rankValue < 1 || rankValue > 13) {
                throw std::invalid_argument("Rank out of range: " + rankStr);
            }
            rank = static_cast<Rank>(rankValue);
        } catch (const std::exception&) {
            throw std::invalid_argument("Invalid rank: " + rankStr);
        }
    }

    return Card(suit, rank);
}

std::vector<std::string> GameManager::convertCardsToStrings(const std::vector<Card>& cards) {
    std::vector<std::string> result;
    for (const Card& card : cards) {
        result.push_back(card.toString());
    }
    return result;
}