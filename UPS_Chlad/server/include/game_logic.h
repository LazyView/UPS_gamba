// game_logic.h - Gamba Card Game Logic
// KIV/UPS Network Programming Project

#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include <vector>
#include <string>
#include <map>
#include <random>
#include <algorithm>
#include <set>
#include <ostream>

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
    bool is_paused = false;
    std::string paused_reason = "";
    std::set<std::string> disconnected_players;
    enum class PlayResult { SUCCESS, INVALID_PLAYER, INVALID_CARD, GAME_OVER, PICKUP_REQUIRED };


    std::string getCurrentPlayer();
    void nextPlayer();
    void initializeDeck();
    void shuffleDeck();
    void dealCards();
    Card getTopCard() const;
    bool canStartGame() const;
    void pauseGame(const std::string& reason);
    void resumeGame();
    bool canResumeGame();
    PlayResult playCards(const std::string& player_id, const std::vector<std::string>& cards);
    bool pickupPile(const std::string& player_id);
    bool canPlayCard(const Card& card, const Card& top_card) const;
    void handleSpecialCardEffects(const Card& card);
};

#endif // GAME_LOGIC_H