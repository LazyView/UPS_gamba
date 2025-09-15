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
    // Check if room is full
    if (room_it->second.players.size() >= 2) {
        return false;
    }
    // Check if player already in room
    if(std::find(room_it->second.players.begin(), room_it->second.players.end(), player_id) != room_it->second.players.end()) {
        return false;
    }
    room_it->second.players.push_back(player_id);
    playerManager->setPlayerRoom(player_id, room_id);
    return true;
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

    auto& room = room_it->second;

    // Check if can start game
    if (room.players.size() != 2) {
        return false;  // Need exactly 2 players
    }

    // TODO: Initialize game when we implement GameLogic/CardDeck
    // room.game_state.dealCards();
    // room.game_state.phase = GamePhase::PLAYING;

    return true;  // For now, just return success
}

bool RoomManager::playCards(const std::string& player_name, const std::vector<std::string>& cards) {
    std::lock_guard<std::mutex> lock(rooms_mutex);

    // Get player's room
    std::string room_id = playerManager->getPlayerRoom(player_name);
    if (room_id.empty()) return false;

    // Find room
    auto room_it = rooms.find(room_id);
    if (room_it == rooms.end()) return false;

    // TODO: Call existing game logic when implemented
    // auto result = room_it->second.game_state.playCards(player_name, cards);
    // return result == GambaGameState::PlayResult::SUCCESS;

    return true;  // For now, just return success
}

bool RoomManager::pickupPile(const std::string& player_name) {
    std::lock_guard<std::mutex> lock(rooms_mutex);

    std::string room_id = playerManager->getPlayerRoom(player_name);
    if (room_id.empty()) return false;

    auto room_it = rooms.find(room_id);
    if (room_it == rooms.end()) return false;

    // TODO: pickupPile returns bool directly when implemented
    // return room_it->second.game_state.pickupPile(player_name);

    return true;  // For now, just return success
}