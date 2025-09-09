// gamba_server.cpp - Main Game Server Implementation
// KIV/UPS Network Programming Project

#include "../include/gamba_server.h"

// Player struct implementation
Player::Player() : id(""), name(""), room_id(""), connected(false) {}

Player::Player(const std::string& player_id, const std::string& player_name)
    : id(player_id), name(player_name), room_id(""), connected(true) {}

// Room struct implementation
Room::Room() : id(""), active(false) {}

Room::Room(const std::string& room_id) : id(room_id), active(true) {}

// GambaServer implementation
GambaServer::GambaServer(const ServerConfig& cfg) : config(cfg), server_socket(-1) {}

// Destructor
GambaServer::~GambaServer() {
    stop();
    if (heartbeat_thread.joinable()) {
        heartbeat_thread.join();
    }
}

void GambaServer::log(const std::string& message) {
    std::lock_guard<std::mutex> lock(cout_mutex);
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);

    std::cout << "[" << std::put_time(&tm, "%H:%M:%S") << "] " << message << std::endl;
}

std::string GambaServer::generatePlayerId() {
    int id = ++player_id_counter;
    return "PLAYER_" + std::to_string(id);
}

std::string GambaServer::generateRoomId() {
    std::lock_guard<std::mutex> lock(rooms_mutex);
    int room_count = rooms.size() + 1;
    return "ROOM_" + std::to_string(room_count);
}

bool GambaServer::sendMessage(int client_socket, const ProtocolMessage& message) {
    std::string serialized = message.serialize() + "\n";
    int bytes_sent = send(client_socket, serialized.c_str(), serialized.length(), 0);
    return bytes_sent > 0;
}

ProtocolMessage GambaServer::handleConnect(const ProtocolMessage& msg, int client_socket) {
    std::string player_name = msg.getData("name", "Anonymous");

    // Check if this is a returning temporarily disconnected player
    {
        std::lock_guard<std::mutex> lock(players_mutex);
        for (auto& pair : players) {
            Player& existing_player = pair.second;
            if (existing_player.name == player_name &&
                existing_player.temporarily_disconnected &&
                !existing_player.connected) {

                // Automatic reconnection for temporary disconnection
                existing_player.connected = true;
                existing_player.temporarily_disconnected = false;
                socket_to_player[client_socket] = existing_player.id;

                {
                    std::lock_guard<std::mutex> hb_lock(heartbeat_mutex);
                    player_last_ping[existing_player.id] = std::chrono::steady_clock::now();
                }

                log("Player automatically reconnected (short-term): " + player_name + " (" + existing_player.id + ")");

                // Try to resume game
                if (!existing_player.room_id.empty()) {
                    resumeGameInRoom(existing_player.room_id, existing_player.id);
                }

                return ProtocolHelper::createConnectedResponse(existing_player.id, player_name);
            }
        }
    }

    std::string player_id = generatePlayerId();

    // DEBUG: Log what we received
    log("DEBUG - Raw message data:");
    for (const auto& pair : msg.data) {
        log("   Key: '" + pair.first + "' = Value: '" + pair.second + "'");
    }
    log("Extracted name: '" + player_name + "'");

    // Add player to server
    {
        std::lock_guard<std::mutex> lock(players_mutex);
        players.emplace(player_id, Player(player_id, player_name));
        socket_to_player[client_socket] = player_id;
    }
    {
        std::lock_guard<std::mutex> hb_lock(heartbeat_mutex);
        player_last_ping[player_id] = std::chrono::steady_clock::now();
    }

    log("Player connected: " + player_name + " (" + player_id + ")");
    return ProtocolHelper::createConnectedResponse(player_id, player_name);
}

ProtocolMessage GambaServer::handleReconnect(const ProtocolMessage& msg, int client_socket) {
    std::string player_id = msg.player_id;
    std::string room_id = msg.room_id;

    if (player_id.empty()) {
        return ProtocolHelper::createErrorResponse("Player ID required for reconnection");
    }

    {
        std::lock_guard<std::mutex> players_lock(players_mutex);

        auto player_it = players.find(player_id);
        if (player_it == players.end()) {
            return ProtocolHelper::createErrorResponse("Player not found");
        }

        Player& player = player_it->second;
        if (player.connected) {
            return ProtocolHelper::createErrorResponse("Player already connected");
        }

        if (player.temporarily_disconnected) {
            return ProtocolHelper::createErrorResponse("Use normal connection for temporary disconnections");
        }

        // Long-term reconnection
        player.connected = true;
        socket_to_player[client_socket] = player_id;
    }

    {
        std::lock_guard<std::mutex> hb_lock(heartbeat_mutex);
        player_last_ping[player_id] = std::chrono::steady_clock::now();
    }

    log("Player manually reconnected (long-term): " + players[player_id].name + " (" + player_id + ")");

    if (!room_id.empty()) {
        resumeGameInRoom(room_id, player_id);
    }

    ProtocolMessage response(MessageType::PLAYER_RECONNECTED);
    response.player_id = player_id;
    response.room_id = room_id;
    response.setData("status", "reconnected");

    return response;
}

ProtocolMessage GambaServer::handleJoinRoom(const ProtocolMessage& msg, int client_socket) {
    std::string player_id = getPlayerIdFromSocket(client_socket);
    if (player_id.empty()) {
        return ProtocolHelper::createErrorResponse("Not connected");
    }

    std::string room_id = msg.room_id;
    if (room_id.empty()) {
        // Create new room
        room_id = generateRoomId();
    }

    // Add player to room
    {
        std::lock_guard<std::mutex> rooms_lock(rooms_mutex);
        std::lock_guard<std::mutex> players_lock(players_mutex);

        // Create room if it doesn't exist
        if (rooms.find(room_id) == rooms.end()) {
            rooms[room_id] = Room(room_id);
            log("Created room: " + room_id);
        }

        // Add player to room
        auto& room = rooms[room_id];
        room.players.push_back(player_id);

        // Update player's room
        players[player_id].room_id = room_id;
    }

    log("Player " + player_id + " joined room " + room_id);
    return ProtocolHelper::createRoomJoinedResponse(player_id, room_id);
}

ProtocolMessage GambaServer::handlePing(const ProtocolMessage& msg) {
    std::string player_id = msg.player_id;
    if (!player_id.empty()) {
        std::lock_guard<std::mutex> lock(heartbeat_mutex);
        player_last_ping[player_id] = std::chrono::steady_clock::now();
    }
    return ProtocolHelper::createPongResponse();
}

ProtocolMessage GambaServer::handleStartGame(const ProtocolMessage& /* msg */, int client_socket) {
    std::string player_id = getPlayerIdFromSocket(client_socket);
    if (player_id.empty()) {
        return ProtocolHelper::createErrorResponse("Not connected");
    }

    std::lock_guard<std::mutex> rooms_lock(rooms_mutex);
    std::lock_guard<std::mutex> players_lock(players_mutex);

    std::string room_id = players[player_id].room_id;
    if (room_id.empty()) {
        return ProtocolHelper::createErrorResponse("Not in a room");
    }

    auto& room = rooms[room_id];
    auto& game_state = room.game_state;

    // Debug output
    log("DEBUG - Room " + room_id + " has " + std::to_string(room.players.size()) + " players");
    log("DEBUG - Game phase: " + std::to_string(static_cast<int>(game_state.phase)));

    if (game_state.phase != GamePhase::WAITING || room.players.size() < 2) {
        log("DEBUG - Can start game: NO");
        return ProtocolHelper::createErrorResponse("Cannot start game (need 2+ players, phase must be WAITING)");
    }

    log("DEBUG - Can start game: YES");

    // Initialize game
    game_state.phase = GamePhase::DEALING;
    game_state.player_order = room.players;
    game_state.current_player_index = 0;

    // Initialize player states
    for (const auto& pid : room.players) {
        game_state.player_states[pid] = PlayerGameState();
    }

    // Deal cards
    game_state.dealCards();
    game_state.phase = GamePhase::PLAYING;

    log("Game started in room " + room_id + " with " +
        std::to_string(room.players.size()) + " players");

    ProtocolMessage response(MessageType::GAME_STARTED);
    response.room_id = room_id;
    response.setData("players", std::to_string(room.players.size()));
    response.setData("current_player", game_state.getCurrentPlayer());
    response.setData("top_card", game_state.getTopCard().toString());

    return response;
}

std::string GambaServer::getPlayerIdFromSocket(int client_socket) {
    std::lock_guard<std::mutex> lock(players_mutex);
    auto it = socket_to_player.find(client_socket);
    return (it != socket_to_player.end()) ? it->second : "";
}

void GambaServer::handleDisconnect(int client_socket) {
    std::string player_id = getPlayerIdFromSocket(client_socket);
    if (!player_id.empty()) {
        std::lock_guard<std::mutex> lock(players_mutex);
        if (players.find(player_id) != players.end()) {
            log("Player disconnected: " + players[player_id].name + " (" + player_id + ")");
            players.erase(player_id);
        }
        socket_to_player.erase(client_socket);
    }
    // Clean up invalid message counter
    {
        std::lock_guard<std::mutex> lock(players_mutex);
        client_invalid_count.erase(client_socket);
    }
}

bool GambaServer::handleInvalidMessage(int client_socket, const std::string& reason) {
    std::lock_guard<std::mutex> lock(players_mutex);

    client_invalid_count[client_socket]++;
    int count = client_invalid_count[client_socket];

    log("Invalid message from socket " + std::to_string(client_socket) +
        " (count: " + std::to_string(count) + "/" +
        std::to_string(config.invalid_message_limit) + "): " + reason);

    if (count >= config.invalid_message_limit) {
        log("Disconnecting socket " + std::to_string(client_socket) +
            " - exceeded invalid message limit");
        return false;  // Signal to disconnect
    }

    return true;  // Continue connection
}

ProtocolMessage GambaServer::processMessage(const std::string& raw_message, int client_socket) {
    // Remove newline if present
    log("RAW MESSAGE (length=" + std::to_string(raw_message.length()) + "): '" + raw_message + "'");
    std::string clean_message = raw_message;
    if (!clean_message.empty() && clean_message.back() == '\n') {
        clean_message.pop_back();
    }
    if (!clean_message.empty() && clean_message.back() == '\r') {
        clean_message.pop_back();
    }
    log("CLEAN MESSAGE (length=" + std::to_string(clean_message.length()) + "): '" + clean_message + "'");

    // Validate message format
    if (!ProtocolHelper::isValidMessage(clean_message)) {
        if (!handleInvalidMessage(client_socket, "Invalid format: " + clean_message)) {
            // Return special disconnect signal
            ProtocolMessage disconnect_msg(MessageType::ERROR_MSG);
            disconnect_msg.setData("disconnect", "true");
            return disconnect_msg;
        }
        return ProtocolHelper::createErrorResponse("Invalid message format");
    }

    // Parse message
    ProtocolMessage msg = ProtocolMessage::parse(clean_message);

    log("Received " + ProtocolHelper::getMessageTypeName(msg.type) + " from socket " + std::to_string(client_socket));

    // Process based on message type
    switch (msg.type) {
        case MessageType::CONNECT:
            return handleConnect(msg, client_socket);

        case MessageType::RECONNECT:
            return handleReconnect(msg, client_socket);

        case MessageType::JOIN_ROOM:
            return handleJoinRoom(msg, client_socket);

        case MessageType::PING:
            return handlePing(msg);

        case MessageType::START_GAME:
            return handleStartGame(msg, client_socket);

        case MessageType::PLAY_CARDS:
            return handlePlayCards(msg, client_socket);

        case MessageType::PICKUP_PILE:
            return handlePickupPile(msg, client_socket);

        default:
            log("Unhandled message type: " + ProtocolHelper::getMessageTypeName(msg.type));
            return ProtocolHelper::createErrorResponse("Unknown message type");
    }
}

void GambaServer::handleClient(int client_socket, std::string client_ip, int client_port) {
    int client_num = ++client_count;
    log("Client #" + std::to_string(client_num) + " connected from "
        + client_ip + ":" + std::to_string(client_port));

    char buffer[1024];

    while (running) {
        // Receive message from client
        int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                log("Client #" + std::to_string(client_num) + " disconnected gracefully");
            } else {
                log("Client #" + std::to_string(client_num) + " connection error");
            }
            break;
        }

        // Null-terminate received data
        buffer[bytes_received] = '\0';
        std::string raw_message(buffer);

        // Process protocol message
        ProtocolMessage response = processMessage(raw_message, client_socket);

        // Check if we should disconnect
        if (response.type == MessageType::ERROR_MSG && response.hasData("disconnect")) {
            log("Disconnecting client due to too many invalid messages");
            break;  // Exit the client loop
        }

        // Send response
        if (!sendMessage(client_socket, response)) {
            log("Failed to send response to Client #" + std::to_string(client_num));
            break;
        }

        log("Sent " + ProtocolHelper::getMessageTypeName(response.type) + " response to Client #" + std::to_string(client_num));
    }

    // Handle disconnection
    handleDisconnect(client_socket);

    // Cleanup
    close(client_socket);
    --client_count;
    log("Client #" + std::to_string(client_num) + " connection closed. Active: " + std::to_string(client_count.load()));
}

bool GambaServer::start() {
    log("=== GAMBA GAME SERVER (Protocol-Enabled) ===");
    log("Starting server on " + config.ip + ":" + std::to_string(config.port) + "...");

    // Create TCP socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        log("Failed to create socket");
        return false;
    }
    log("Socket created successfully");
    log("Starting heartbeat monitor...");
    startHeartbeatMonitor();
    log("Heartbeat monitor started");
    // Allow socket reuse
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Configure server address using config
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(config.port);

    // Bind socket
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        log("Failed to bind socket to port " + std::to_string(config.port));
        close(server_socket);
        return false;
    }
    log("Socket bound to port " + std::to_string(config.port));

    // Start listening
    if (listen(server_socket, 10) < 0) {
        log("Failed to listen on socket");
        close(server_socket);
        return false;
    }

    log("Server ready - Protocol enabled!");
    log("Supported messages: CONNECT, JOIN_ROOM, PING, START_GAME");
    log("Test with: telnet localhost " + std::to_string(config.port));
    log("Example: 0|||name=Alice");

    return true;
}

/*
This method takes care server running a
*/
void GambaServer::run() {
    while (running) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        int client_socket = accept(server_socket,
                                 (struct sockaddr*)&client_addr,
                                 &client_addr_len);

        if (client_socket < 0) {
            if (running) {
                log("Failed to accept client connection");
            }
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        int client_port = ntohs(client_addr.sin_port);

        std::thread client_thread(&GambaServer::handleClient, this,
                                client_socket, std::string(client_ip), client_port);
        client_thread.detach();

        log("Active connections: " + std::to_string(client_count.load()));
    }
}

/*
This will stop server!
*/
void GambaServer::stop() {
    if (running) {
        running = false;
        if (server_socket != -1) {
            close(server_socket);
            server_socket = -1;
        }
        log("Server stopped");
    }
}

/*
Loop for heartbeat check
*/
void GambaServer::startHeartbeatMonitor() {
    heartbeat_thread = std::thread([this]() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(10));
            checkHeartbeats();
        }
    });
}
/*
Function that checks if client is alive, send ping every 30s and should receive pong, if not
game will pause until player reconnects
*/
void GambaServer::checkHeartbeats() {
    std::lock_guard<std::mutex> hb_lock(heartbeat_mutex);
    std::lock_guard<std::mutex> players_lock(players_mutex);

    auto now = std::chrono::steady_clock::now();
    log("Checking heartbeats for " + std::to_string(players.size()) + " players");

    for (auto& pair : players) {
        Player& player = pair.second;
        log("Player " + player.id + " connected: " + (player.connected ? "YES" : "NO"));

        if (player.connected) {
            auto ping_it = player_last_ping.find(player.id);
            if (ping_it != player_last_ping.end()) {
                auto time_since_ping = now - ping_it->second;
                auto seconds = std::chrono::duration_cast<std::chrono::seconds>(time_since_ping).count();
                log("Player " + player.id + " last ping: " + std::to_string(seconds) + " seconds ago");

                if (time_since_ping > std::chrono::seconds(120)) {
                    log("Player " + player.id + " - short-term disconnection detected");
                    player.temporarily_disconnected = true;
                    player.disconnection_start = now;
                    pauseGameInRoom(player.room_id, player.id);
                    player.connected = false;
                }
            }
        } else if (player.temporarily_disconnected) {
            // Check if temporary disconnection becomes permanent
            auto disconnection_time = now - player.disconnection_start;
            auto disconnection_seconds = std::chrono::duration_cast<std::chrono::seconds>(disconnection_time).count();

            if (disconnection_time > std::chrono::seconds(240)) {  // 90 seconds = long-term
                log("Player " + player.id + " - long-term disconnection detected (requires manual reconnect)");
                player.temporarily_disconnected = false;
                // Player remains disconnected and requires RECONNECT message
            } else {
                log("Player " + player.id + " - temporarily disconnected for " + std::to_string(disconnection_seconds) + " seconds");
            }
        }
    }
}

void GambaServer::pauseGameInRoom(const std::string& room_id, const std::string& disconnected_player) {
    if (room_id.empty()) return;

    std::lock_guard<std::mutex> lock(rooms_mutex);
    auto room_it = rooms.find(room_id);
    if (room_it != rooms.end()) {
        auto& game_state = room_it->second.game_state;

        if (game_state.phase == GamePhase::PLAYING) {
            game_state.pauseGame("Player " + disconnected_player + " disconnected");
            game_state.disconnected_players.insert(disconnected_player);

            log("Game paused in room " + room_id + " - player " + disconnected_player + " disconnected");

            // Notify remaining players
            //broadcastGamePaused(room_id, disconnected_player);
        }
    }
}

void GambaServer::resumeGameInRoom(const std::string& room_id, const std::string& reconnected_player) {
    if (room_id.empty()) return;

    std::lock_guard<std::mutex> lock(rooms_mutex);
    auto room_it = rooms.find(room_id);
    if (room_it != rooms.end()) {
        auto& room = room_it->second;
        auto& game_state = room.game_state;

        game_state.disconnected_players.erase(reconnected_player);

        log("DEBUG - Disconnected players remaining: " + std::to_string(game_state.disconnected_players.size()));
        log("DEBUG - Game paused: " + std::string(game_state.is_paused ? "YES" : "NO"));

        if (game_state.canResumeGame()) {
            game_state.resumeGame();
            log("Game resumed in room " + room_id + " - all players reconnected");
        } else {
            log("Game still paused in room " + room_id + " - waiting for other players");
        }
    }
}

ProtocolMessage GambaServer::handlePlayCards(const ProtocolMessage& msg, int client_socket) {
    std::string player_id = getPlayerIdFromSocket(client_socket);
    if (player_id.empty()) {
        return ProtocolHelper::createErrorResponse("Not connected");
    }

    std::lock_guard<std::mutex> rooms_lock(rooms_mutex);
    std::lock_guard<std::mutex> players_lock(players_mutex);

    std::string room_id = players[player_id].room_id;
    if (room_id.empty()) {
        return ProtocolHelper::createErrorResponse("Not in a room");
    }

    auto& room = rooms[room_id];
    auto& game_state = room.game_state;

    if (game_state.phase != GamePhase::PLAYING) {
        return ProtocolHelper::createErrorResponse("Game not in progress");
    }

    // Parse cards from message data
    std::vector<std::string> cards;
    if (msg.data.count("cards")) {
        std::istringstream iss(msg.data.at("cards"));
        std::string card;
        while (std::getline(iss, card, ',')) {
            cards.push_back(card);
        }
    }

    // Execute card play
    auto result = game_state.playCards(player_id, cards);

    // Create response
    ProtocolMessage response(MessageType::TURN_RESULT);
    response.player_id = player_id;
    response.room_id = room_id;
    response.setData("result", std::to_string(static_cast<int>(result)));

    // Broadcast game state to all players if successful
    if (result == GambaGameState::PlayResult::SUCCESS) {
        broadcastGameState(room_id);

        // Check if game is over
        if (game_state.phase == GamePhase::FINISHED) {
            ProtocolMessage game_over(MessageType::GAME_OVER);
            game_over.room_id = room_id;
            game_over.setData("winner", player_id);
            // TODO: Broadcast game over to all players
        }
    }

    return response;
}

ProtocolMessage GambaServer::handlePickupPile(const ProtocolMessage& msg, int client_socket) {
    std::string player_id = getPlayerIdFromSocket(client_socket);
    if (player_id.empty()) {
        return ProtocolHelper::createErrorResponse("Not connected");
    }

    std::lock_guard<std::mutex> rooms_lock(rooms_mutex);
    std::lock_guard<std::mutex> players_lock(players_mutex);

    std::string room_id = players[player_id].room_id;
    if (room_id.empty()) {
        return ProtocolHelper::createErrorResponse("Not in a room");
    }

    auto& room = rooms[room_id];
    auto& game_state = room.game_state;

    if (game_state.pickupPile(player_id)) {
        broadcastGameState(room_id);

        ProtocolMessage response(MessageType::TURN_RESULT);
        response.player_id = player_id;
        response.room_id = room_id;
        response.setData("result", "pickup_success");
        return response;
    } else {
        return ProtocolHelper::createErrorResponse("Cannot pickup pile");
    }
}

void GambaServer::broadcastGameState(const std::string& room_id) {
    auto& room = rooms[room_id];
    auto& game_state = room.game_state;

    for (const auto& player_id : room.players) {
        auto socket_it = std::find_if(socket_to_player.begin(), socket_to_player.end(),
            [&player_id](const auto& pair) { return pair.second == player_id; });

        if (socket_it != socket_to_player.end()) {
            ProtocolMessage msg(MessageType::GAME_STATE);
            msg.room_id = room_id;
            msg.player_id = player_id;

            // Add game state data
            msg.setData("phase", std::to_string(static_cast<int>(game_state.phase)));
            msg.setData("current_player", game_state.getCurrentPlayer());
            msg.setData("top_card", game_state.getTopCard().toString());
            msg.setData("discard_size", std::to_string(game_state.discard_pile.size()));
            msg.setData("deck_empty", game_state.deck.empty() ? "true" : "false");

            // Player-specific data
            if (game_state.player_states.count(player_id)) {
                auto& player_state = game_state.player_states[player_id];
                msg.setData("hand", player_state.getHandString());
                msg.setData("reserve_count", std::to_string(player_state.reserve_cards.size()));
                msg.setData("is_turn", player_state.is_turn ? "true" : "false");
            }

            sendMessage(socket_it->first, msg);
        }
    }
}

































