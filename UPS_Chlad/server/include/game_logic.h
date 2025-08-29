// game_logic.h - Gamba Card Game Logic
// KIV/UPS Network Programming Project

#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include <vector>
#include <string>
#include <map>
#include <random>
#include <algorithm>

// Card representation
struct Card {
    enum Suit { HEARTS, DIAMONDS, CLUBS, SPADES };
    enum Value { ACE = 1, TWO = 2, THREE = 3, FOUR = 4, FIVE = 5, SIX = 6,
                SEVEN = 7, EIGHT = 8, NINE = 9, TEN = 10, JACK = 11, QUEEN = 12, KING = 13 };

    Suit suit;
    Value value;

    Card(Suit s, Value v);
    std::string toString() const;
    bool isSpecial() const;
};

// Player game state
struct PlayerGameState {
    std::vector<Card> hand;
    std::vector<Card> reserve_cards;
    bool is_turn = false;

    bool hasWon() const;
    std::string getHandString() const;
};

// Game phases
enum class GamePhase { WAITING, DEALING, PLAYING, FINISHED };

// Main game state management
struct GambaGameState {
    GamePhase phase = GamePhase::WAITING;
    std::vector<std::string> player_order;
    int current_player_index = 0;
    std::vector<Card> deck;
    std::vector<Card> discard_pile;
    std::map<std::string, PlayerGameState> player_states;

    std::string getCurrentPlayer();
    void nextPlayer();
    void initializeDeck();
    void shuffleDeck();
    void dealCards();
    Card getTopCard() const;
    bool canStartGame() const;
};

#endif // GAME_LOGIC_H