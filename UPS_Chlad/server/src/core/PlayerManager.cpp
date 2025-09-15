//
// Created by chlad on 9/11/2025.
//

#include "PlayerManager.h"

Player::Player(const std::string& player_name, int socket)
    : name(player_name), socket_fd(socket), connected(true),
      room_id(""), temporarily_disconnected(false) {
}

std::string PlayerManager::connectPlayer(const std::string& player_name, int client_socket) {
    std::lock_guard<std::mutex> lock(players_mutex);

    auto it = players.find(player_name);

    if (it != players.end()) {
        if (it->second.connected) {
            return "";  // Already connected
        } else {
            // Reconnect existing player
            it->second.connected = true;
            it->second.socket_fd = client_socket;
            socket_to_player[client_socket] = player_name;
            // Update heartbeat here for simplicity
            updateLastPing(player_name);

            return player_name;
        }
    } else {
        // Add new player
        players.emplace(player_name, Player(player_name, client_socket));
        socket_to_player[client_socket] = player_name;
        updateLastPing(player_name);
        return player_name;
    }
}

std::string PlayerManager::getPlayerIdFromSocket(int client_socket) {
    std::lock_guard<std::mutex> lock(players_mutex);

    auto it = socket_to_player.find(client_socket);
    if (it != socket_to_player.end()) {
        return it->second;  // Return player name
    }
    return "";  // Socket not found
}

/**
* Complete delete from the server
**/
void PlayerManager::removePlayer(const std::string& player_name) {
    std::lock_guard<std::mutex> lock(players_mutex);

    auto it = players.find(player_name);
    if (it != players.end()) {
        // Get socket before removing player
        int socket_fd = it->second.socket_fd;

        // Remove from socket mapping (if socket is valid)
        if (socket_fd != -1) {
            socket_to_player.erase(socket_fd);
        }

        // Remove player completely
        players.erase(it);

        // Clean up heartbeat data
        {
            std::lock_guard<std::mutex> hb_lock(heartbeat_mutex);
            player_last_ping.erase(player_name);
        }
    }
}

void PlayerManager::updateLastPing(const std::string& player_name) {
    // Check player exists first
    {
        std::lock_guard<std::mutex> lock(players_mutex);
        if (players.find(player_name) == players.end()) {
            return; // Player doesn't exist
        }
    }

    // Update ping
    {
        std::lock_guard<std::mutex> lock(heartbeat_mutex);
        player_last_ping[player_name] = std::chrono::steady_clock::now();
    }
}

std::optional<Player> PlayerManager::getPlayer(const std::string& player_name) {
    std::lock_guard<std::mutex> lock(players_mutex);

    auto it = players.find(player_name);
    if (it != players.end()) {
        return it->second;  // Return copy
    }
    return std::nullopt;  // Not found
}

void PlayerManager::markPlayerDisconnected(const std::string& player_name) {
    std::lock_guard<std::mutex> lock(players_mutex);

    auto it = players.find(player_name);
    if (it != players.end()) {
        // Get socket before clearing it
        int socket_fd = it->second.socket_fd;

        // Update player state
        it->second.connected = false;
        it->second.socket_fd = -1;
        it->second.temporarily_disconnected = true;
        it->second.disconnection_start = std::chrono::steady_clock::now();

        // Remove from socket mapping (if socket was valid)
        if (socket_fd != -1) {
            socket_to_player.erase(socket_fd);
        }
    }
}

// ROOM MANAGEMENT

void PlayerManager::setPlayerRoom(const std::string& player_name, const std::string& room_id) {
    std::lock_guard<std::mutex> lock(players_mutex);

    auto it = players.find(player_name);
    if (it != players.end()) {
        it->second.room_id = room_id;
    }
}

std::string PlayerManager::getPlayerRoom(const std::string& player_name) {
    std::lock_guard<std::mutex> lock(players_mutex);

    auto it = players.find(player_name);
    if (it != players.end()) {
        return it->second.room_id;
    }
    return "";  // Player not found
}

void PlayerManager::clearPlayerRoom(const std::string& player_name) {
    std::lock_guard<std::mutex> lock(players_mutex);

    auto it = players.find(player_name);
    if (it != players.end()) {
        it->second.room_id = "";  // Empty string = lobby
    }
}

bool PlayerManager::playerExists(const std::string& player_name) {
    std::lock_guard<std::mutex> lock(players_mutex);
    return players.find(player_name) != players.end();
}

std::size_t PlayerManager::getPlayerCount(){
    std::lock_guard<std::mutex> lock(players_mutex);
    return players.size();
}

/**
*
*/
std::vector<std::string> PlayerManager::getPlayersInRoom(const std::string& room_id) {
    std::lock_guard<std::mutex> lock(players_mutex);
    std::vector<std::string> roomPlayers;
    for (const auto& pair : players) {
        if (pair.second.room_id == room_id) {
            roomPlayers.push_back(pair.first);
        }
    }
    return roomPlayers;
}

/**
*    Returns list of player names.
*/
std::vector<std::string> PlayerManager::getAllPlayers() {
    std::lock_guard<std::mutex> lock(players_mutex);
    std::vector<std::string> playerNames;
    for (const auto& pair : players) {
        playerNames.push_back(pair.first);
    }
    return playerNames;
}