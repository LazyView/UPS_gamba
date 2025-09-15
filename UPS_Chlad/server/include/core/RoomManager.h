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
#include "PlayerManager.h"

class RoomManager {
private:
    std::mutex rooms_mutex;
    std::map<std::string, Room> rooms;
    PlayerManager* playerManager;  // Add reference to PlayerManager
    int next_room_id = 1;          // Add missing member variable

public:
    // Constructor to inject PlayerManager dependency
    RoomManager(PlayerManager* pm) : playerManager(pm) {}

    std::string createRoom();                                    // Returns new room ID
    bool deleteRoom(const std::string& room_id);
    bool joinRoom(const std::string& player_name, const std::string& room_id);
    bool leaveRoom(const std::string& player_name);

    // Simplified queries
    bool roomExists(const std::string& room_id);
    bool isRoomFull(const std::string& room_id);               // True if 2 players
    std::vector<std::string> getRoomPlayers(const std::string& room_id);
    size_t getRoomCount();
    std::string joinAnyAvailableRoom(const std::string& player_name);  // Fix declaration
    bool startGame(const std::string& room_id);
    bool playCards(const std::string& player_name, const std::vector<std::string>& cards);
    bool pickupPile(const std::string& player_name);
};


#endif //ROOMMANAGER_H