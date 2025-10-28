#ifndef PLAYER_H
#define PLAYER_H

struct Player {
    std::string name;
    std::string room_id;
    bool connected;
    int socket_fd;  // -1 when disconnected

    std::chrono::steady_clock::time_point last_ping;
    std::chrono::steady_clock::time_point disconnection_start;
    bool temporarily_disconnected;

    // Simple constructor
    Player(const std::string& player_name, int socket);
};

#endif //PLAYER_h