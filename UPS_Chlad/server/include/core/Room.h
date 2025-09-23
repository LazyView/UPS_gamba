//
// Created by chlad on 9/11/2025.
//

#ifndef ROOM_H
#define ROOM_H

#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include "../game/GameLogic.h"

struct Room {
    std::string id;
    std::vector<std::string> players;
    bool active;
    std::unique_ptr<GameLogic> gameLogic;

    // Add constructor
    Room() : id(""), active(false), gameLogic(std::make_unique<GameLogic>()) {}
    Room(const std::string& room_id) : id(room_id), active(false), gameLogic(std::make_unique<GameLogic>()) {}

    // Copy constructor and assignment operator for proper unique_ptr handling
    Room(const Room& other)
        : id(other.id), players(other.players), active(other.active),
          gameLogic(std::make_unique<GameLogic>(*other.gameLogic)) {}

    Room& operator=(const Room& other) {
        if (this != &other) {
            id = other.id;
            players = other.players;
            active = other.active;
            gameLogic = std::make_unique<GameLogic>(*other.gameLogic);
        }
        return *this;
    }

    // Move constructor and assignment operator
    Room(Room&& other) noexcept
        : id(std::move(other.id)), players(std::move(other.players)),
          active(other.active), gameLogic(std::move(other.gameLogic)) {}

    Room& operator=(Room&& other) noexcept {
        if (this != &other) {
            id = std::move(other.id);
            players = std::move(other.players);
            active = other.active;
            gameLogic = std::move(other.gameLogic);
        }
        return *this;
    }

    // Game management methods
    bool addPlayerToGame(const std::string& playerId) {
        // Add to room player list
        auto it = std::find(players.begin(), players.end(), playerId);
        if (it == players.end()) {
            players.push_back(playerId);
        }

        // Add to game logic
        return gameLogic->addPlayer(playerId);
    }

    bool removePlayerFromGame(const std::string& playerId) {
        // Remove from room player list
        auto it = std::find(players.begin(), players.end(), playerId);
        if (it != players.end()) {
            players.erase(it);
        }

        // Remove from game logic
        return gameLogic->removePlayer(playerId);
    }

    bool startGame() {
        if (players.size() < 2) {
            return false;
        }

        try {
            gameLogic->startGame();
            active = true;
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }

    void resetGame() {
        gameLogic->resetGame();
        active = false;
    }

    bool isGameActive() const {
        return active && gameLogic->getGameState() == GameState::GAME_STARTED;
    }

    bool isGameFinished() const {
        return gameLogic->getGameState() == GameState::GAME_FINISHED;
    }
};

#endif //ROOM_H