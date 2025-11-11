#ifndef PLAYERMANAGER_H
#define PLAYERMANAGER_H

#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <chrono>
#include <cstddef>  // for size_t
#include <optional>
#include "Player.h"

class PlayerManager {
private:
    std::mutex players_mutex;
    std::map<std::string, Player> players;
    std::map<int, std::string> socket_to_player;

    // Heartbeat tracking
    std::map<std::string, std::chrono::steady_clock::time_point> player_last_ping;
    std::mutex heartbeat_mutex;  // Separate mutex for heartbeat operations

public:
    // Player lifecycle
    std::string connectPlayer(const std::string& player_name, int client_socket);
    void removePlayer(const std::string& player_name);
    void markPlayerTemporarilyDisconnected(const std::string& player_name);
    void removeSocketMapping(int client_socket);
    bool reconnectPlayer(const std::string& player_name, int new_socket);

    // Player lookup
    std::string getPlayerIdFromSocket(int client_socket);
    std::optional<Player> getPlayer(const std::string& player_name);
    bool playerExists(const std::string& player_name);
    std::vector<std::pair<std::string, bool>> getPlayersForHeartbeatCheck();
    std::vector<std::string> getDisconnectedPlayersForCleanup(int cleanup_seconds);

    // Connection management
    void updatePlayerConnection(const std::string& player_name, bool connected);
    void updatePlayerSocket(const std::string& player_name, int new_socket);
    void markSocketDisconnected(const std::string& player_name);  // Mark socket closed, but wait for timeout before temp disconnect

    // Room management
    void setPlayerRoom(const std::string& player_name, const std::string& room_id);
    std::string getPlayerRoom(const std::string& player_name);
    void clearPlayerRoom(const std::string& player_name);

    // Heartbeat management (from current server)
    void updateLastPing(const std::string& player_name);
    std::chrono::steady_clock::time_point getLastPing(const std::string& player_name);
    void markPlayerDisconnected(const std::string& player_name);
    void markReconnected(const std::string& player_name);

    // Timeout checking
    std::vector<std::string> getTimedOutPlayers(int timeout_seconds);
    void cleanupTimedOutPlayers(int timeout_seconds);

    // Utility
    std::vector<std::string> getAllPlayers();
    std::vector<std::string> getPlayersInRoom(const std::string& room_id);
    size_t getPlayerCount();

    // Cleanup
    void cleanup();  // For server shutdown
};

#endif //PLAYERMANAGER_H