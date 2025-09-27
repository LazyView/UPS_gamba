//
// Created by chlad on 9/11/2025.
//

#include "RoomManager.h"
#include <mutex>
#include <algorithm>  // for std::find, std::remove
#include <vector>
#include <string>
#include <iostream>

std::string RoomManager::createRoom() {
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

    auto room_it = rooms.find(room_id);
    if (room_it == rooms.end()) {
        return false;  // Room doesn't exist
    }

    Room& room = room_it->second;

    // Check if room is full
    if (room.players.size() >= 2) {  // FIX: was calling isRoomFull incorrectly
        return false;
    }

    // Check if player already in room
    if(std::find(room.players.begin(), room.players.end(), player_id) != room.players.end()) {
        return false;
    }

    // ADD THE MISSING PART:
    if (room.addPlayerToGame(player_id)) {
        return true;
    }

    return false;
}

bool RoomManager::isRoomFull(const std::string& room_id) {

    auto room_it = rooms.find(room_id);
    if (room_it == rooms.end()) {
        return false;  // Room doesn't exist, so not full
    }

    return room_it->second.players.size() >= 2;  // True if exactly 2 players
}

bool RoomManager::leaveRoom(const std::string& player_id, const std::string& room_id) {
    std::lock_guard<std::mutex> lock(rooms_mutex);

    if (room_id.empty()) {
        return false;  // Player not in any room
    }

    auto room_it = rooms.find(room_id);
    if (room_it != rooms.end()) {
        // Remove player from room's player list
        auto& players_vec = room_it->second.players;
        players_vec.erase(std::remove(players_vec.begin(), players_vec.end(), player_id), players_vec.end());

        // Delete room if empty
        if (room_it->second.players.empty()) {
            rooms.erase(room_it);
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

    std::cout << "DEBUG: Looking for available room for player: " << player_name << std::endl;
    std::cout << "DEBUG: Total rooms: " << rooms.size() << std::endl;

    // Look for rooms with exactly 1 player (available for second player)
    for (auto& pair : rooms) {
        std::cout << "DEBUG: Room " << pair.first << " has " << pair.second.players.size() << " players" << std::endl;

        if (pair.second.players.size() == 1) {
            std::cout << "DEBUG: Found available room: " << pair.first << std::endl;
            // Found room with 1 player - join it
            if (joinRoom(player_name, pair.first)) {
                std::cout << "DEBUG: Successfully joined room: " << pair.first << std::endl;
                return pair.first;  // Return room_id
            } else {
                std::cout << "DEBUG: Failed to join room: " << pair.first << std::endl;
            }
        }
    }

    // No available rooms - create new empty room
    std::cout << "DEBUG: No available rooms, creating new room" << std::endl;
    std::string new_room = createRoom();
    if (joinRoom(player_name, new_room)) {
        std::cout << "DEBUG: Created and joined new room: " << new_room << std::endl;
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

void RoomManager::handlePlayerTimeout(const std::string& player_name, const std::string& room_id) {
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
    }
}