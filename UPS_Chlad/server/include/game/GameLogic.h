//
// Created by chlad on 9/11/2025.
//

#ifndef GAMELOGIC_H
#define GAMELOGIC_H

#include "CardDeck.h"
#include <vector>
#include <string>
#include <map>

enum class GameState {
    WAITING_FOR_PLAYERS,
    GAME_STARTED,
    GAME_FINISHED
};

struct PlayerHand {
    std::vector<Card> hand;           // Cards in hand (up to 3)
    std::vector<Card> reserves;       // Face-down reserve cards (up to 3)
    std::string playerId;

    PlayerHand(const std::string& id) : playerId(id) {}
};

class GameLogic {
private:
    CardDeck deck;
    std::vector<Card> discardPile;
    std::vector<PlayerHand> players;
    std::map<std::string, size_t> playerIndexMap;

    size_t currentPlayerIndex;
    GameState gameState;
    bool clockwise;                   // Direction of play
    bool mustPlaySevenOrLower;        // Special state after 7 is played

public:
    GameLogic();

    // Game setup
    bool addPlayer(const std::string& playerId);
    bool removePlayer(const std::string& playerId);
    void startGame();
    void resetGame();

    // Game state queries
    GameState getGameState() const;
    std::string getCurrentPlayer() const;
    size_t getPlayerCount() const;
    bool isPlayerInGame(const std::string& playerId) const;

    // Card dealing and drawing
    void dealInitialCards();
    void drawCardsToHand(const std::string& playerId);

    // Game actions
    bool playCards(const std::string& playerId, const std::vector<Card>& cardsToPlay);
    bool pickupDiscardPile(const std::string& playerId);

    // Turn management
    void nextTurn();
    bool isPlayerTurn(const std::string& playerId) const;

    // Win condition
    bool hasPlayerWon(const std::string& playerId) const;
    std::string getWinner() const;

    // Game state access
    std::vector<Card> getPlayerHand(const std::string& playerId) const;
    std::vector<Card> getPlayerReserves(const std::string& playerId) const;
    size_t getPlayerHandSize(const std::string& playerId) const;
    size_t getPlayerReserveSize(const std::string& playerId) const;
    std::vector<Card> getDiscardPile() const;
    Card getTopDiscardCard() const;
    size_t getDeckSize() const;
    bool getMustPlaySevenOrLower() const;

    // Utility methods
    void shuffleDiscardPileIntoDeck();

private:
    size_t getPlayerIndex(const std::string& playerId) const;
    void recycleDiscardPile();
    void moveToNextValidPlayer();
};

#endif //GAMELOGIC_H