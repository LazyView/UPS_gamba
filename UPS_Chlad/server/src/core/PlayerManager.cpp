//
// Created by chlad on 9/11/2025.
//

#include "PlayerManager.h"

Player::Player(const std::string& player_name, int socket)
    : name(player_name), socket_fd(socket), connected(true),
      room_id(""), temporarily_disconnected(false) {
}

std::string PlayerManager::connectPlayer(const std::string& player_name, int client_socket) {
    auto it = players.find(player_name);

    if (it != players.end()) {
        // Player already exists - reject connection
        // They must use RECONNECT message instead
        return "";
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

std::chrono::steady_clock::time_point PlayerManager::getLastPing(const std::string& player_name) {
    std::lock_guard<std::mutex> lock(heartbeat_mutex);

    auto it = player_last_ping.find(player_name);
    if (it != player_last_ping.end()) {
        return it->second;
    }
    return std::chrono::steady_clock::time_point{}; // Return default-constructed time_point if not found
}

void PlayerManager::markReconnected(const std::string& player_name) {
    std::lock_guard<std::mutex> lock(players_mutex);

    auto it = players.find(player_name);
    if (it != players.end()) {
        it->second.connected = true;
        it->second.temporarily_disconnected = false;
        // Note: socket_fd should be set externally when reconnecting
    }
}

std::vector<std::string> PlayerManager::getTimedOutPlayers(int timeout_seconds) {
    std::vector<std::string> timed_out_players;
    auto now = std::chrono::steady_clock::now();
    auto timeout_duration = std::chrono::seconds(timeout_seconds);

    {
        std::lock_guard<std::mutex> players_lock(players_mutex);
        std::lock_guard<std::mutex> heartbeat_lock(heartbeat_mutex);

        for (const auto& pair : players) {
            const std::string& player_name = pair.first;
            const Player& player = pair.second;

            // Only check connected players for timeout
            if (player.connected) {
                auto ping_it = player_last_ping.find(player_name);
                if (ping_it != player_last_ping.end()) {
                    auto time_since_last_ping = now - ping_it->second;
                    if (time_since_last_ping > timeout_duration) {
                        timed_out_players.push_back(player_name);
                    }
                }
            }
        }
    }

    return timed_out_players;
}

void PlayerManager::cleanupTimedOutPlayers(int timeout_seconds) {
    std::vector<std::string> timed_out_players = getTimedOutPlayers(timeout_seconds);

    for (const std::string& player_name : timed_out_players) {
        markPlayerDisconnected(player_name);
    }
}

void PlayerManager::markPlayerTemporarilyDisconnected(const std::string& player_name) {
    std::lock_guard<std::mutex> lock(players_mutex);

    auto it = players.find(player_name);
    if (it != players.end()) {
        int socket_fd = it->second.socket_fd;

        // Update player state for temporary disconnection
        it->second.connected = false;
        it->second.socket_fd = -1;
        it->second.temporarily_disconnected = true;
        it->second.disconnection_start = std::chrono::steady_clock::now();

        // Remove from socket mapping
        if (socket_fd != -1) {
            socket_to_player.erase(socket_fd);
        }
    }
}

void PlayerManager::removeSocketMapping(int client_socket) {
    std::lock_guard<std::mutex> lock(players_mutex);

    auto it = socket_to_player.find(client_socket);
    if (it != socket_to_player.end()) {
        socket_to_player.erase(it);
    }
}

bool PlayerManager::reconnectPlayer(const std::string& player_name, int new_socket) {
    std::lock_guard<std::mutex> lock(players_mutex);

    auto it = players.find(player_name);
    if (it != players.end() && it->second.temporarily_disconnected) {
        // Restore connection
        it->second.connected = true;
        it->second.socket_fd = new_socket;
        it->second.temporarily_disconnected = false;
        socket_to_player[new_socket] = player_name;

        // Update heartbeat
        updateLastPing(player_name);
        return true;
    }

    return false; // Player not found or not in reconnection state
}

std::vector<std::pair<std::string, bool>> PlayerManager::getPlayersForHeartbeatCheck() {
    std::lock_guard<std::mutex> lock(players_mutex);
    std::vector<std::pair<std::string, bool>> result;

    for (const auto& pair : players) {
        result.emplace_back(pair.first, pair.second.connected);
    }
    return result;
}

std::vector<std::string> PlayerManager::getDisconnectedPlayersForCleanup(int cleanup_seconds) {
    std::lock_guard<std::mutex> lock(players_mutex);
    std::vector<std::string> cleanup_players;
    auto now = std::chrono::steady_clock::now();
    auto cleanup_timeout = std::chrono::seconds(cleanup_seconds);

    for (const auto& pair : players) {
        const Player& player = pair.second;
        if (player.temporarily_disconnected) {
            auto time_since_disconnect = now - player.disconnection_start;
            if (time_since_disconnect > cleanup_timeout) {
                cleanup_players.push_back(pair.first);
            }
        }
    }
    return cleanup_players;
}