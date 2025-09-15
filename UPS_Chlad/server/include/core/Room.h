//
// Created by chlad on 9/11/2025.
//

#ifndef ROOM_H
#define ROOM_H

#include <string>
#include <vector>
// #include "GameLogic.h"
struct Room {
    std::string id;
    std::vector<std::string> players;
    bool active;
    // GambaGameState game_state;  // Comment out for now

    // Add constructor
    Room() : id(""), active(false) {}
    Room(const std::string& room_id) : id(room_id), active(false) {}
};

#endif //ROOM_H