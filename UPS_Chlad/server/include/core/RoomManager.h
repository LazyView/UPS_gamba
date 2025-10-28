//
// Created by chlad on 9/11/2025.
//

#ifndef ROOMMANAGER_H
#define ROOMMANAGER_H

#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <cstddef>  // for size_t
#include "Room.h"
#include "../game/CardDeck.h"

class RoomManager {
private:
    std::mutex rooms_mutex;
    std::map<std::string, Room> rooms;
    int next_room_id = 1;          // Add missing member variable

public:
    // Constructor to inject PlayerManager dependency
    RoomManager() {}

    std::string createRoom();                                    // Returns new room ID
    bool deleteRoom(const std::string& room_id);
    bool joinRoom(const std::string& player_name, const std::string& room_id);
    bool leaveRoom(const std::string& player_id, const std::string& room_id);

    // Simplified queries
    bool roomExists(const std::string& room_id);
    bool isRoomFull(const std::string& room_id);               // True if 2 players
    std::vector<std::string> getRoomPlayers(const std::string& room_id);
    size_t getRoomCount();
    std::string joinAnyAvailableRoom(const std::string& player_name);  // Fix declaration
    bool startGame(const std::string& room_id);

    // Player timeout handling
    void handlePlayerTimeout(const std::string& player_name, const std::string& room_id);

    // Add this template method for safe room access
    template<typename GameOperation>
    auto withRoom(const std::string& room_id, GameOperation operation) {
        std::lock_guard<std::mutex> lock(rooms_mutex);
        auto room_it = rooms.find(room_id);
        if (room_it == rooms.end()) {
            return operation(nullptr);  // Pass nullptr for invalid room
        }
        return operation(&room_it->second);  // Pass room reference safely
    }

private:
};


#endif //ROOMMANAGER_H