//
// Created by chlad on 9/11/2025.
//

#include "GameLogic.h"
#include "GameRules.h"
#include <algorithm>
#include <stdexcept>
#include <cstdint>
#include <iostream>

GameLogic::GameLogic()
    : currentPlayerIndex(0), gameState(GameState::WAITING_FOR_PLAYERS),
      clockwise(true), mustPlaySevenOrLower(false) {
}

bool GameLogic::addPlayer(const std::string& playerId) {
    if (gameState != GameState::WAITING_FOR_PLAYERS) {
        return false;
    }

    if (isPlayerInGame(playerId)) {
        return false; // Player already in game
    }

    players.emplace_back(playerId);
    playerIndexMap[playerId] = players.size() - 1;
    return true;
}

bool GameLogic::removePlayer(const std::string& playerId) {
    if (gameState == GameState::GAME_STARTED) {
        return false; // Cannot remove during active game
    }

    auto it = playerIndexMap.find(playerId);
    if (it == playerIndexMap.end()) {
        return false;
    }

    size_t index = it->second;
    players.erase(players.begin() + index);
    playerIndexMap.erase(it);

    // Rebuild index map
    playerIndexMap.clear();
    for (size_t i = 0; i < players.size(); ++i) {
        playerIndexMap[players[i].playerId] = i;
    }

    return true;
}

void GameLogic::startGame() {
    if (players.size() < 2) {
        throw std::runtime_error("Need at least 2 players to start game");
    }

    gameState = GameState::GAME_STARTED;
    currentPlayerIndex = 0;
    clockwise = true;
    mustPlaySevenOrLower = false;

    // Initialize deck and shuffle
    deck.initializeStandardDeck();
    deck.shuffle();

    // Clear discard pile
    discardPile.clear();

    dealInitialCards();
}

void GameLogic::dealInitialCards() {
    // Deal 6 cards to each player (3 reserves + 3 hand)
    for (auto& player : players) {
        player.hand.clear();
        player.reserves.clear();

        // Deal 3 reserve cards (face down)
        for (int i = 0; i < 3; ++i) {
            if (!deck.isEmpty()) {
                player.reserves.push_back(deck.dealCard());
            }
        }

        // Deal 3 hand cards
        for (int i = 0; i < 3; ++i) {
            if (!deck.isEmpty()) {
                player.hand.push_back(deck.dealCard());
            }
        }
    }

    // Place first card on discard pile
    if (!deck.isEmpty()) {
        discardPile.push_back(deck.dealCard());
    }
}

void GameLogic::drawCardsToHand(const std::string& playerId) {
    size_t playerIndex = getPlayerIndex(playerId);
    if (playerIndex >= players.size()) return;

    PlayerHand& player = players[playerIndex];

    // Draw cards to maintain 3 in hand (if deck has cards)
    while (player.hand.size() < 3 && !deck.isEmpty()) {
        player.hand.push_back(deck.dealCard());
    }
    
    // Note: If deck is empty, player just doesn't draw
    // They continue playing with whatever cards they have
}

bool GameLogic::playCards(const std::string& playerId, const std::vector<Card>& cardsToPlay) {
    if (!isPlayerTurn(playerId) || cardsToPlay.empty()) {
        return false;
    }

    size_t playerIndex = getPlayerIndex(playerId);
    if (playerIndex >= players.size()) return false;

    PlayerHand& player = players[playerIndex];

    // Validate cards are in player's hand
    for (const Card& card : cardsToPlay) {
        auto it = std::find_if(player.hand.begin(), player.hand.end(),
            [&card](const Card& handCard) {
                return handCard.suit == card.suit && handCard.rank == card.rank;
            });
        if (it == player.hand.end()) {
            return false; // Card not in hand
        }
    }

    // Validate play using GameRules
    // IMPORTANT: If discard pile is empty (after pickup), any card is valid
    if (!discardPile.empty()) {
        Card topCard = getTopDiscardCard();
        if (!GameRules::isValidPlay(cardsToPlay, topCard, mustPlaySevenOrLower)) {
            return false;
        }
    }
    // If pile is empty, skip validation - any card is valid

    // Remove cards from hand
    for (const Card& card : cardsToPlay) {
        auto it = std::find_if(player.hand.begin(), player.hand.end(),
            [&card](const Card& handCard) {
                return handCard.suit == card.suit && handCard.rank == card.rank;
            });
        if (it != player.hand.end()) {
            player.hand.erase(it);
        }
    }
    // Add cards to discard pile
    for (const Card& card : cardsToPlay) {
        discardPile.push_back(card);
    }

    // Handle special card effects
    GameRules::applySpecialCardEffects(cardsToPlay, discardPile, clockwise, mustPlaySevenOrLower);

    // Draw cards back to 3 (if deck has cards)
    drawCardsToHand(playerId);

    // Check for win condition
    if (hasPlayerWon(playerId)) {
        gameState = GameState::GAME_FINISHED;
        return true;
    }

    nextTurn();
    return true;
}

bool GameLogic::pickupDiscardPile(const std::string& playerId) {
    if (!isPlayerTurn(playerId) || discardPile.empty()) {
        return false;
    }

    size_t playerIndex = getPlayerIndex(playerId);
    if (playerIndex >= players.size()) return false;

    PlayerHand& player = players[playerIndex];

    // Add all discard pile cards to player's hand
    player.hand.insert(player.hand.end(), discardPile.begin(), discardPile.end());
    discardPile.clear();

    // Reset special states
    mustPlaySevenOrLower = false;

    nextTurn();
    return true;
}

void GameLogic::nextTurn() {
    moveToNextValidPlayer();
}

void GameLogic::moveToNextValidPlayer() {
    if (players.empty()) return;

    if (clockwise) {
        currentPlayerIndex = (currentPlayerIndex + 1) % players.size();
    } else {
        currentPlayerIndex = (currentPlayerIndex == 0) ? players.size() - 1 : currentPlayerIndex - 1;
    }
}

bool GameLogic::isPlayerTurn(const std::string& playerId) const {
    if (gameState != GameState::GAME_STARTED) return false;
    if (currentPlayerIndex >= players.size()) return false;
    return players[currentPlayerIndex].playerId == playerId;
}

bool GameLogic::hasPlayerWon(const std::string& playerId) const {
    size_t playerIndex = getPlayerIndex(playerId);
    if (playerIndex >= players.size()) return false;

    const PlayerHand& player = players[playerIndex];
    return player.hand.empty() && player.reserves.empty();
}

std::string GameLogic::getWinner() const {
    if (gameState != GameState::GAME_FINISHED) return "";

    for (const auto& player : players) {
        if (player.hand.empty() && player.reserves.empty()) {
            return player.playerId;
        }
    }
    return "";
}

void GameLogic::resetGame() {
    gameState = GameState::WAITING_FOR_PLAYERS;
    players.clear();
    playerIndexMap.clear();
    discardPile.clear();
    currentPlayerIndex = 0;
    clockwise = true;
    mustPlaySevenOrLower = false;
    deck.initializeStandardDeck();
}

// Getters
GameState GameLogic::getGameState() const {
    return gameState;
}

std::string GameLogic::getCurrentPlayer() const {
    if (currentPlayerIndex >= players.size()) return "";
    return players[currentPlayerIndex].playerId;
}

size_t GameLogic::getPlayerCount() const {
    return players.size();
}

bool GameLogic::isPlayerInGame(const std::string& playerId) const {
    return playerIndexMap.find(playerId) != playerIndexMap.end();
}

std::vector<Card> GameLogic::getPlayerHand(const std::string& playerId) const {
    size_t playerIndex = getPlayerIndex(playerId);
    if (playerIndex >= players.size()) return {};
    return players[playerIndex].hand;
}

std::vector<Card> GameLogic::getPlayerReserves(const std::string& playerId) const {
    size_t playerIndex = getPlayerIndex(playerId);
    if (playerIndex >= players.size()) return {};
    return players[playerIndex].reserves;
}

size_t GameLogic::getPlayerHandSize(const std::string& playerId) const {
    size_t playerIndex = getPlayerIndex(playerId);
    if (playerIndex >= players.size()) return 0;
    return players[playerIndex].hand.size();
}

size_t GameLogic::getPlayerReserveSize(const std::string& playerId) const {
    size_t playerIndex = getPlayerIndex(playerId);
    if (playerIndex >= players.size()) return 0;
    return players[playerIndex].reserves.size();
}

std::vector<Card> GameLogic::getDiscardPile() const {
    return discardPile;
}

Card GameLogic::getTopDiscardCard() const {
    if (discardPile.empty()) {
        throw std::runtime_error("Discard pile is empty");
    }
    return discardPile.back();
}

size_t GameLogic::getDeckSize() const {
    return deck.size();
}

bool GameLogic::getMustPlaySevenOrLower() const {
    return mustPlaySevenOrLower;
}

void GameLogic::shuffleDiscardPileIntoDeck() {
    recycleDiscardPile();
}

// Private methods
size_t GameLogic::getPlayerIndex(const std::string& playerId) const {
    auto it = playerIndexMap.find(playerId);
    return (it != playerIndexMap.end()) ? it->second : SIZE_MAX;
}

void GameLogic::recycleDiscardPile() {
    if (discardPile.size() <= 1) return;

    // Keep the top card, shuffle the rest back into deck
    Card topCard = discardPile.back();
    discardPile.pop_back();

    deck.addCards(discardPile);
    deck.shuffle();

    discardPile.clear();
    discardPile.push_back(topCard);
}

bool GameLogic::playFromReserve(const std::string& playerId) {
    if (!isPlayerTurn(playerId)) {
        return false;
    }

    size_t playerIndex = getPlayerIndex(playerId);
    if (playerIndex >= players.size()) return false;

    PlayerHand& player = players[playerIndex];

    // Check if player can play from reserves
    if (!player.hand.empty()) {
        return false;  // Must have empty hand to play from reserves
    }

    if (player.reserves.empty()) {
        return false;  // No reserves left
    }

    // Randomly select one reserve card (take from back - "blind" selection)
    Card reserve_card = player.reserves.back();
    player.reserves.pop_back();

    // Check if reserve card is valid
    // IMPORTANT: If discard pile is empty (after pickup), any card is valid
    bool isValid = false;
    std::vector<Card> singleCard;
    if (discardPile.empty()) {
        singleCard = {reserve_card};
        isValid = true;  // Any card is valid when pile is empty
    } else {
        Card topCard = getTopDiscardCard();
        singleCard = {reserve_card};
        isValid = GameRules::isValidPlay(singleCard, topCard, mustPlaySevenOrLower);
    }

    if (isValid) {
        // Valid - play the reserve card
        discardPile.push_back(reserve_card);

        // Apply special effects if it's a special card
        GameRules::applySpecialCardEffects(singleCard, discardPile, clockwise, mustPlaySevenOrLower);

        // Check for win condition
        if (hasPlayerWon(playerId)) {
            gameState = GameState::GAME_FINISHED;
            return true;
        }

        nextTurn();
        return true;
    } else {
        // Invalid - player picks up entire pile + the invalid reserve card
        player.hand.push_back(reserve_card);  // Add reserve to hand
        player.hand.insert(player.hand.end(), discardPile.begin(), discardPile.end());  // Add pile
        discardPile.clear();
        mustPlaySevenOrLower = false;  // Reset special state

        nextTurn();
        return true;
    }
}