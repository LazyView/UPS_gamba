//
// Created by chlad on 9/11/2025.
//

#include "MessageHandler.h"
#include "MessageParser.h"
#include "ProtocolHelper.h"
#include "MessageValidator.h"
#include "PlayerManager.h"
#include "RoomManager.h"
#include "GameManager.h"
#include "Logger.h"
#include <sstream>
#include <vector>

MessageHandler::MessageHandler(PlayerManager* pm, RoomManager* rm, MessageValidator* mv, Logger* lg, GameManager* gm)
    : playerManager(pm), roomManager(rm), validator(mv), logger(lg), gameManager(gm) {}

std::vector<ProtocolMessage> MessageHandler::processMessage(const std::string& raw_message, int client_socket) {
    if (!validator->isValidFormat(raw_message)) {
        logger->warning("Invalid message format from socket " + std::to_string(client_socket) + ": '" + raw_message + "'");
        ProtocolMessage disconnect_response(MessageType::ERROR_MSG);
        disconnect_response.setData("disconnect", "true");
        return {disconnect_response};
    }

    ProtocolMessage msg = ProtocolMessage::parse(raw_message);
    logger->debug("Parsed message type: " + std::to_string(static_cast<int>(msg.getType())));
    
    if (!validator->isValidMessageType(static_cast<int>(msg.getType()))) {
        logger->warning("Invalid message type " + std::to_string(static_cast<int>(msg.getType())) + " from socket " + std::to_string(client_socket));
        ProtocolMessage disconnect_response(MessageType::ERROR_MSG);
        disconnect_response.setData("disconnect", "true");
        return {disconnect_response};
    }

    // Player resolution
    std::string player_name = "";
    logger->debug("About to check if message requires active player");

    if (MessageParser::requiresActivePlayer(msg.getType())) {
        logger->debug("Message requires active player, getting from socket");
        player_name = playerManager->getPlayerIdFromSocket(client_socket);
        logger->debug("Got player name from socket: '" + player_name + "'");

        if (player_name.empty()) {
            logger->debug("Player name is empty, returning error");
            return {ProtocolHelper::createErrorResponse("Must connect first")};
        }
    }
    
    logger->debug("About to enter switch statement with message type: " + std::to_string(static_cast<int>(msg.getType())));

    switch (msg.getType()) {
        case MessageType::CONNECT:
            logger->debug("Routing to handleConnect");
            return handleConnect(msg, client_socket);
        case MessageType::RECONNECT:
            logger->debug("Routing to handleReconnect");
            return handleReconnect(msg, client_socket);
        case MessageType::PING:
            logger->debug("Routing to handlePing");
            return handlePing(player_name);
        case MessageType::JOIN_ROOM:
            logger->debug("Routing to handleJoinRoom");
            return handleJoinRoom(player_name);
        case MessageType::LEAVE_ROOM:
            logger->debug("Routing to handleLeaveRoom");
            return handleLeaveRoom(player_name);
        case MessageType::START_GAME:
            logger->debug("Routing to handleStartGame");
            return handleStartGame(player_name);
        case MessageType::PLAY_CARDS:
            logger->debug("Routing to handlePlayCards");
            return handlePlayCards(msg, player_name);
        case MessageType::PICKUP_PILE:
            logger->debug("Routing to handlePickupPile");
            return handlePickupPile(player_name);
        default:
            logger->debug("Unknown message type");
            return {ProtocolHelper::createErrorResponse("Unknown message type")};
    }
}

std::vector<ProtocolMessage> MessageHandler::handlePing(const std::string& player_name) {
    playerManager->updateLastPing(player_name);
    return {ProtocolHelper::createPongResponse()};
}

std::vector<ProtocolMessage> MessageHandler::handleConnect(const ProtocolMessage& msg, int client_socket) {
    // 1. Get player name from message
    std::string player_name = MessageParser::getPlayerNameFromMessage(msg);
    logger->debug("handleConnect: extracted player name '" + player_name + "'");
    
    // 2. VALIDATE PLAYER NAME
    if (player_name.empty()) {
        logger->warning("handleConnect: empty player name");
        return {ProtocolHelper::createErrorResponse("Player name cannot be empty")};
    }
    
    if (player_name.length() > 32) {
        logger->warning("handleConnect: player name too long (" + std::to_string(player_name.length()) + " chars)");
        return {ProtocolHelper::createErrorResponse("Player name too long (max 32 characters)")};
    }
    
    // Check for invalid characters (only allow alphanumeric, underscore, hyphen)
    for (char c : player_name) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-') {
            logger->warning("handleConnect: player name contains invalid character: '" + std::string(1, c) + "'");
            return {ProtocolHelper::createErrorResponse("Player name contains invalid characters (only letters, numbers, _, - allowed)")};
        }
    }
    
    // 3. Try to connect player
    logger->debug("handleConnect: calling playerManager->connectPlayer with name='" + player_name + "', socket=" + std::to_string(client_socket));
    std::string result = playerManager->connectPlayer(player_name, client_socket);
    logger->debug("handleConnect: playerManager->connectPlayer returned '" + result + "'");

    // 4. Return response
    if (!result.empty()) {
        logger->debug("handleConnect: creating success response");
        return {ProtocolHelper::createConnectedResponse(result, player_name)};
    } else {
        logger->debug("handleConnect: creating error response");
        return {ProtocolHelper::createErrorResponse("Connection failed - name already taken")};
    }
}

std::vector<ProtocolMessage> MessageHandler::handleJoinRoom(const std::string& player_name) {
    logger->debug("handleJoinRoom: called for player '" + player_name + "'");
    
    // DEBUG: Check player state
    auto player_opt = playerManager->getPlayer(player_name);
    if (player_opt.has_value()) {
        logger->debug("  Player found: connected=" + std::string(player_opt->connected ? "true" : "false") + 
                     ", room_id='" + player_opt->room_id + "', socket=" + std::to_string(player_opt->socket_fd));
    } else {
        logger->error("  Player not found in PlayerManager!");
    }
    
    std::string assigned_room = roomManager->joinAnyAvailableRoom(player_name);

    if (!assigned_room.empty()) {
        playerManager->setPlayerRoom(player_name, assigned_room);
        logger->debug("handleJoinRoom: room assigned successfully: " + assigned_room);

        // Get list of all players currently in the room
        std::vector<std::string> room_players = roomManager->getRoomPlayers(assigned_room);
        
        // Build comma-separated player list
        std::string players_list;
        for (size_t i = 0; i < room_players.size(); ++i) {
            if (i > 0) players_list += ",";
            players_list += room_players[i];
        }
        
        logger->debug("handleJoinRoom: players in room: " + players_list + " (count: " + std::to_string(room_players.size()) + ")");

        // Create response with player list
        ProtocolMessage response = ProtocolHelper::createRoomJoinedResponse(player_name, assigned_room);
        response.setData("players", players_list);
        response.setData("player_count", std::to_string(room_players.size()));
        response.setData("room_full", (room_players.size() >= 2) ? "true" : "false");
        response.should_broadcast_to_room = true;
        
        logger->info("Player '" + player_name + "' joined room '" + assigned_room + "' with " + std::to_string(room_players.size()) + " players");
        
        return {response};
    } else {
        logger->debug("handleJoinRoom: room not assigned");
        return {ProtocolHelper::createErrorResponse("Error occurred while joining room")};
    }
}

std::vector<ProtocolMessage> MessageHandler::handleReconnect(const ProtocolMessage& msg, int client_socket) {
    std::string player_name = MessageParser::getPlayerNameFromMessage(msg);

    if (player_name.empty()) {
        return {ProtocolHelper::createErrorResponse("Player name required")};
    }

    // Try to reconnect existing temporarily disconnected player
    if (playerManager->reconnectPlayer(player_name, client_socket)) {
        logger->info("Player '" + player_name + "' reconnected successfully");
        
        std::vector<ProtocolMessage> responses;
        
        // 1. Send CONNECTED response to reconnecting player
        ProtocolMessage connected = ProtocolHelper::createConnectedResponse(player_name, player_name);
        connected.player_id = player_name;
        responses.push_back(connected);
        
        // 2. Get player's room
        std::string room_id = playerManager->getPlayerRoom(player_name);
        
        if (!room_id.empty() && room_id != "lobby") {
            logger->info("Player '" + player_name + "' was in room '" + room_id + "', restoring state");
            
            // 3. Check if game is active in the room
            if (gameManager->isGameActive(roomManager, room_id)) {
                logger->info("Game is active, sending current game state to '" + player_name + "'");
                
                // Send current game state to reconnected player
                try {
                    GameStateData game_data = gameManager->getGameStateForPlayer(roomManager, room_id, player_name);
                    
                    if (game_data.valid) {
                        ProtocolMessage game_state = ProtocolHelper::createGameStateResponse(player_name, room_id, game_data);
                        game_state.player_id = player_name;
                        responses.push_back(game_state);
                        logger->debug("Sent game state to reconnected player '" + player_name + "'");
                    }
                } catch (const std::exception& e) {
                    logger->error("Failed to get game state for reconnected player: " + std::string(e.what()));
                }
                
                // 4. Notify other players in room about reconnection
                std::vector<std::string> room_players = roomManager->getRoomPlayers(room_id);
                for (const std::string& other_player : room_players) {
                    if (other_player != player_name) {
                        ProtocolMessage reconnect_notification(MessageType::PLAYER_RECONNECTED);
                        reconnect_notification.player_id = other_player;
                        reconnect_notification.room_id = room_id;
                        reconnect_notification.setData("reconnected_player", player_name);
                        reconnect_notification.setData("status", "reconnected");
                        responses.push_back(reconnect_notification);
                    }
                }
            } else {
                logger->info("No active game in room '" + room_id + "'");
            }
        } else {
            logger->info("Player '" + player_name + "' was in lobby, no game state to restore");
        }
        
        return responses;
    } else {
        logger->warning("Reconnection failed for player '" + player_name + "' - not found or not disconnected");
        return {ProtocolHelper::createErrorResponse("Reconnection failed - player not found or session expired")};
    }
}

std::vector<ProtocolMessage> MessageHandler::handleLeaveRoom(const std::string& player_name) {
    std::string room_id = playerManager->getPlayerRoom(player_name);
    bool result = roomManager->leaveRoom(player_name, room_id);

    if (result) {
        playerManager->clearPlayerRoom(player_name);
        ProtocolMessage response = ProtocolHelper::createRoomLeftResponse(player_name);
        response.should_broadcast_to_room = true;
        return {response};
    } else {
        return {ProtocolHelper::createErrorResponse("Leave room failed")};
    }
}

std::vector<ProtocolMessage> MessageHandler::handleStartGame(const std::string& player_name) {
    std::string room_id = playerManager->getPlayerRoom(player_name);

    if (room_id.empty()) {
        return {ProtocolHelper::createErrorResponse("Not in any room")};
    }

    // Start game via GameManager
    bool result = gameManager->startGame(roomManager, room_id);

    if (!result) {
        return {ProtocolHelper::createErrorResponse("Cannot start game")};
    }
    
    logger->info("Game started in room '" + room_id + "' by player '" + player_name + "'");
    
    // Prepare multiple messages
    std::vector<ProtocolMessage> responses;
    
    // 1. GAME_STARTED broadcast
    ProtocolMessage game_started = ProtocolHelper::createGameStartedResponse();
    game_started.room_id = room_id;
    game_started.should_broadcast_to_room = true;
    responses.push_back(game_started);
    
    // 2. GAME_STATE for each player in room
    std::vector<std::string> room_players = roomManager->getRoomPlayers(room_id);
    
    for (const std::string& target_player : room_players) {
        try {
            // Get game state data from GameManager
            GameStateData game_data = gameManager->getGameStateForPlayer(roomManager, room_id, target_player);
            
            if (game_data.valid) {
                // Convert to ProtocolMessage
                ProtocolMessage game_state = ProtocolHelper::createGameStateResponse(target_player, room_id, game_data);
                game_state.player_id = target_player;  // Mark who this is for
                responses.push_back(game_state);
                logger->debug("Added game state for player '" + target_player + "'");
            } else {
                logger->error("Invalid game state for player '" + target_player + "': " + game_data.error_message);
            }
        } catch (const std::exception& e) {
            logger->error("Failed to get game state for player '" + target_player + "': " + e.what());
        }
    }
    
    logger->info("Returning " + std::to_string(responses.size()) + " messages for game start");
    
    return responses;
}

std::vector<ProtocolMessage> MessageHandler::handlePickupPile(const std::string& player_name) {
    // 1. Get player's room
    std::string room_id = playerManager->getPlayerRoom(player_name);
    if (room_id.empty()) {
        return {ProtocolHelper::createErrorResponse("Not in any room")};
    }

    // 2. Execute pickup via GameManager
    bool result = gameManager->pickupPile(roomManager, room_id, player_name);

    if (!result) {
        return {ProtocolHelper::createErrorResponse("Cannot pickup pile")};
    }
    
    // 3. Success! Build responses
    std::vector<ProtocolMessage> responses;
    
    // a. Send TURN_RESULT confirmation
    ProtocolMessage turn_result = ProtocolHelper::createTurnResultResponse("pickup_success");
    turn_result.player_id = player_name;
    responses.push_back(turn_result);
    
    // b. Send updated GAME_STATE to all players
    std::vector<std::string> room_players = roomManager->getRoomPlayers(room_id);
    
    for (const std::string& target_player : room_players) {
        try {
            GameStateData game_data = gameManager->getGameStateForPlayer(roomManager, room_id, target_player);
            
            if (game_data.valid) {
                ProtocolMessage game_state = ProtocolHelper::createGameStateResponse(target_player, room_id, game_data);
                game_state.player_id = target_player;
                responses.push_back(game_state);
                logger->debug("Added game state for player '" + target_player + "'");
            } else {
                logger->error("Invalid game state for player '" + target_player + "': " + game_data.error_message);
            }
        } catch (const std::exception& e) {
            logger->error("Failed to get game state for player '" + target_player + "': " + e.what());
        }
    }
    
    logger->info("Player '" + player_name + "' picked up pile, returning " + std::to_string(responses.size()) + " messages");
    
    return responses;
}

std::vector<ProtocolMessage> MessageHandler::handlePlayCards(const ProtocolMessage& msg, const std::string& player_name) {
    // 1. Extract cards from message
    std::string cards_str = MessageParser::extractDataField(msg, "cards");
    if (cards_str.empty()) {
        return {ProtocolHelper::createErrorResponse("No cards specified")};
    }

    // 2. Parse comma-separated cards
    std::vector<std::string> cards;
    std::stringstream ss(cards_str);
    std::string card;
    while (std::getline(ss, card, ',')) {
        cards.push_back(card);
        logger->debug("Parsed card from message: '" + card + "'");
    }
    
    logger->info("Player '" + player_name + "' attempting to play " + std::to_string(cards.size()) + " cards: " + cards_str);

    // 3. Get player's room
    std::string room_id = playerManager->getPlayerRoom(player_name);
    if (room_id.empty()) {
        return {ProtocolHelper::createErrorResponse("Not in any room")};
    }

    // 4. Try to play cards via GameManager (handles both normal and reserve plays)
    bool result = gameManager->playCards(roomManager, room_id, player_name, cards);

    if (!result) {
        return {ProtocolHelper::createErrorResponse("Invalid card play")};
    }
    
    // 5. Success! Build responses
    std::vector<ProtocolMessage> responses;
    
    // a. Send TURN_RESULT confirmation to player who played
    ProtocolMessage turn_result = ProtocolHelper::createTurnResultResponse("play_success");
    turn_result.player_id = player_name;  // Targeted to player
    responses.push_back(turn_result);
    
    // b. Check if game ended (works for both normal cards AND reserve cards)
    if (gameManager->isGameOver(roomManager, room_id)) {
        std::string winner = gameManager->getWinner(roomManager, room_id);
        logger->info("Game over in room '" + room_id + "', winner: " + winner);
        
        // Get room players before cleanup
        std::vector<std::string> room_players = roomManager->getRoomPlayers(room_id);
        
        // Send GAME_OVER to both players
        for (const std::string& target_player : room_players) {
            ProtocolMessage game_over = ProtocolHelper::createGameOverResponse(winner);
            game_over.player_id = target_player;
            game_over.room_id = room_id;
            responses.push_back(game_over);
        }
        
        // Send ROOM_LEFT to both players (they're back in lobby)
        for (const std::string& target_player : room_players) {
            ProtocolMessage room_left = ProtocolHelper::createRoomLeftResponse(target_player);
            room_left.player_id = target_player;
            responses.push_back(room_left);
            
            // Clear player's room assignment
            playerManager->clearPlayerRoom(target_player);
        }
        
        // Delete the room
        roomManager->deleteRoom(room_id);
        
        logger->info("Room '" + room_id + "' deleted, players returned to lobby");
        logger->info("Returning " + std::to_string(responses.size()) + " messages (game over)");
        return responses;
    }
    
    // c. Send updated GAME_STATE to all players
    std::vector<std::string> room_players = roomManager->getRoomPlayers(room_id);
    
    for (const std::string& target_player : room_players) {
        try {
            GameStateData game_data = gameManager->getGameStateForPlayer(roomManager, room_id, target_player);
            
            if (game_data.valid) {
                ProtocolMessage game_state = ProtocolHelper::createGameStateResponse(target_player, room_id, game_data);
                game_state.player_id = target_player;
                responses.push_back(game_state);
                logger->debug("Added game state for player '" + target_player + "'");
            } else {
                logger->error("Invalid game state for player '" + target_player + "': " + game_data.error_message);
            }
        } catch (const std::exception& e) {
            logger->error("Failed to get game state for player '" + target_player + "': " + e.what());
        }
    }
    
    logger->info("Player '" + player_name + "' played cards, returning " + std::to_string(responses.size()) + " messages");
    
    return responses;
}
