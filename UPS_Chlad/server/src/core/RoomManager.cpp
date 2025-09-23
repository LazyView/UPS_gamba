//
// Created by chlad on 9/11/2025.
//

#include "RoomManager.h"
#include <mutex>
#include <algorithm>  // for std::find, std::remove
#include <vector>
#include <string>

std::string RoomManager::createRoom() {
    std::lock_guard<std::mutex> lock(rooms_mutex);
    std::string room_name = "ROOM_" + std::to_string(next_room_id);
    rooms[room_name] = Room(room_name);
    next_room_id++;
    return room_name;
}

bool RoomManager::roomExists(const std::string& room_id) {
    std::lock_guard<std::mutex> lock(rooms_mutex);
    return rooms.find(room_id) != rooms.end();
}

bool RoomManager::joinRoom(const std::string& player_id, const std::string& room_id) {
    std::lock_guard<std::mutex> lock(rooms_mutex);
    auto room_it = rooms.find(room_id);

    if (room_it == rooms.end()) {
        return false;  // Room doesn't exist
    }

    Room& room = room_it->second;

    // Check if room is full
    if (room.players.size() >= 2) {
        return false;
    }

    // Check if player already in room
    if(std::find(room.players.begin(), room.players.end(), player_id) != room.players.end()) {
        return false;
    }

    // Use the Room's integrated game logic to add player
    if (room.addPlayerToGame(player_id)) {
        playerManager->setPlayerRoom(player_id, room_id);
        return true;
    }

    return false;
}

bool RoomManager::isRoomFull(const std::string& room_id) {
    std::lock_guard<std::mutex> lock(rooms_mutex);

    auto room_it = rooms.find(room_id);
    if (room_it == rooms.end()) {
        return false;  // Room doesn't exist, so not full
    }

    return room_it->second.players.size() == 2;  // True if exactly 2 players
}

bool RoomManager::leaveRoom(const std::string& player_id) {
    std::lock_guard<std::mutex> lock(rooms_mutex);

    auto player = playerManager->getPlayer(player_id);
    if (!player.has_value()) {
        return false;  // Player doesn't exist
    }

    std::string room_id = player->room_id;
    if (room_id.empty()) {
        return false;  // Player not in any room
    }

    auto room_it = rooms.find(room_id);
    if (room_it != rooms.end()) {  // Room exists
        // Remove player from room's player list (use std::remove)
        auto& players_vec = room_it->second.players;
        players_vec.erase(std::remove(players_vec.begin(), players_vec.end(), player_id), players_vec.end());

        // Clear player's room in PlayerManager
        playerManager->clearPlayerRoom(player_id);

        // Delete room if empty
        if (room_it->second.players.empty()) {
            rooms.erase(room_id);
        }
        return true;
    }
    return false;
}

std::vector<std::string> RoomManager::getRoomPlayers(const std::string& room_id) {
    std::lock_guard<std::mutex> lock(rooms_mutex);
    auto room_it = rooms.find(room_id);
    if (room_it == rooms.end()) {
        return std::vector<std::string>();
    }
    return room_it->second.players;
}

std::string RoomManager::joinAnyAvailableRoom(const std::string& player_name) {
    std::lock_guard<std::mutex> lock(rooms_mutex);

    // Look for rooms with exactly 1 player (available for second player)
    for (auto& pair : rooms) {
        if (pair.second.players.size() == 1) {
            // Found room with 1 player - join it
            // This will make it full (2 players)
            if (joinRoom(player_name, pair.first)) {
                return pair.first;  // Return room_id
            }
        }
    }

    // No available rooms - create new empty room
    std::string new_room = createRoom();
    if (joinRoom(player_name, new_room)) {
        return new_room;
    }
    return "";  // Failed to join any room
}

bool RoomManager::startGame(const std::string& room_id) {
    std::lock_guard<std::mutex> lock(rooms_mutex);

    auto room_it = rooms.find(room_id);
    if (room_it == rooms.end()) {
        return false;  // Room not found
    }

    Room& room = room_it->second;

    // Check if can start game
    if (room.players.size() < 2) {
        return false;  // Need at least 2 players
    }

    // Use the Room's integrated game logic
    return room.startGame();
}

bool RoomManager::playCards(const std::string& player_name, const std::vector<std::string>& cards) {
    std::lock_guard<std::mutex> lock(rooms_mutex);

    // Get player's room
    std::string room_id = playerManager->getPlayerRoom(player_name);
    if (room_id.empty()) return false;

    // Find room
    auto room_it = rooms.find(room_id);
    if (room_it == rooms.end()) return false;

    Room& room = room_it->second;

    // Convert string cards to Card objects
    std::vector<Card> cardObjects;
    for (const std::string& cardStr : cards) {
        Card card = parseCardFromString(cardStr);
        if (card.rank == Rank::ACE && card.suit == Suit::HEARTS && cardStr != "AH") {
            // Invalid card string - parseCardFromString returns AH for invalid cards
            return false;
        }
        cardObjects.push_back(card);
    }

    // Use game logic to play cards
    return room.gameLogic->playCards(player_name, cardObjects);
}

bool RoomManager::pickupPile(const std::string& player_name) {
    std::lock_guard<std::mutex> lock(rooms_mutex);

    std::string room_id = playerManager->getPlayerRoom(player_name);
    if (room_id.empty()) return false;

    auto room_it = rooms.find(room_id);
    if (room_it == rooms.end()) return false;

    Room& room = room_it->second;

    // Use game logic to pickup discard pile
    return room.gameLogic->pickupDiscardPile(player_name);
}

Card RoomManager::parseCardFromString(const std::string& cardStr) {
    if (cardStr.length() < 2) {
        // Invalid card string, return default (Ace of Hearts)
        return Card(Suit::HEARTS, Rank::ACE);
    }

    // Parse rank (first character(s))
    Rank rank;
    char rankChar = cardStr[0];
    switch (rankChar) {
        case 'A': rank = Rank::ACE; break;
        case '2': rank = Rank::TWO; break;
        case '3': rank = Rank::THREE; break;
        case '4': rank = Rank::FOUR; break;
        case '5': rank = Rank::FIVE; break;
        case '6': rank = Rank::SIX; break;
        case '7': rank = Rank::SEVEN; break;
        case '8': rank = Rank::EIGHT; break;
        case '9': rank = Rank::NINE; break;
        case '1': // 10 is represented as "10X"
            if (cardStr.length() >= 3 && cardStr[1] == '0') {
                rank = Rank::TEN;
            } else {
                return Card(Suit::HEARTS, Rank::ACE); // Invalid
            }
            break;
        case 'J': rank = Rank::JACK; break;
        case 'Q': rank = Rank::QUEEN; break;
        case 'K': rank = Rank::KING; break;
        default:
            return Card(Suit::HEARTS, Rank::ACE); // Invalid
    }

    // Parse suit (last character)
    char suitChar = cardStr.back();
    Suit suit;
    switch (suitChar) {
        case 'H': suit = Suit::HEARTS; break;
        case 'D': suit = Suit::DIAMONDS; break;
        case 'C': suit = Suit::CLUBS; break;
        case 'S': suit = Suit::SPADES; break;
        default:
            return Card(Suit::HEARTS, Rank::ACE); // Invalid
    }

    return Card(suit, rank);
}

void RoomManager::handlePlayerTimeout(const std::string& player_name) {
    // Get player's room before removing them
    std::string room_id;
    if (playerManager) {
        room_id = playerManager->getPlayerRoom(player_name);
    }

    // If player was in a room, handle the timeout in that room
    if (!room_id.empty() && room_id != "lobby") {
        std::lock_guard<std::mutex> lock(rooms_mutex);
        auto room_it = rooms.find(room_id);

        if (room_it != rooms.end()) {
            Room& room = room_it->second;

            // Remove player from room
            auto it = std::find(room.players.begin(), room.players.end(), player_name);
            if (it != room.players.end()) {
                room.players.erase(it);

                // If room becomes empty, mark it for cleanup
                if (room.players.empty()) {
                    rooms.erase(room_it);
                } else {
                    // If game was in progress and only one player remains, reset the game
                    if (room.isGameActive() && room.players.size() == 1) {
                        room.resetGame();
                        // The remaining player should be returned to waiting state
                    }
                }
            }
        }

        // Clear player's room in PlayerManager
        if (playerManager) {
            playerManager->clearPlayerRoom(player_name);
        }
    }
}