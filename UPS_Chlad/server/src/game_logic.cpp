// game_logic.cpp - Gamba Card Game Logic Implementation
// KIV/UPS Network Programming Project

#include "../include/game_logic.h"

// Card implementation
Card::Card(Suit s, Value v) : suit(s), value(v) {}

std::string Card::toString() const {
    std::string result = std::to_string(static_cast<int>(value));
    switch(suit) {
        case HEARTS: result += "H"; break;
        case DIAMONDS: result += "D"; break;
        case CLUBS: result += "C"; break;
        case SPADES: result += "S"; break;
    }
    return result;
}

bool Card::isSpecial() const {
    return value == TWO || value == SEVEN || value == TEN;
}

// PlayerGameState implementation
bool PlayerGameState::hasWon() const {
    return hand.empty() && reserve_cards.empty();
}

std::string PlayerGameState::getHandString() const {
    std::string result = "[";
    for (size_t i = 0; i < hand.size(); i++) {
        if (i > 0) result += ",";
        result += hand[i].toString();
    }
    result += "]";
    return result;
}

// GambaGameState implementation
std::string GambaGameState::getCurrentPlayer() {
    if (player_order.empty()) return "";
    return player_order[current_player_index];
}

void GambaGameState::nextPlayer() {
    if (!player_order.empty()) {
        // Clear current player's turn flag
        if (player_states.find(getCurrentPlayer()) != player_states.end()) {
            player_states[getCurrentPlayer()].is_turn = false;
        }

        current_player_index = (current_player_index + 1) % player_order.size();

        // Set new current player's turn flag
        if (player_states.find(getCurrentPlayer()) != player_states.end()) {
            player_states[getCurrentPlayer()].is_turn = true;
        }
    }
}

void GambaGameState::initializeDeck() {
    deck.clear();
    for (int suit = 0; suit < 4; suit++) {
        for (int value = 1; value <= 13; value++) {
            deck.emplace_back(static_cast<Card::Suit>(suit), static_cast<Card::Value>(value));
        }
    }
    shuffleDeck();
}

void GambaGameState::shuffleDeck() {
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(deck.begin(), deck.end(), g);
}

void GambaGameState::dealCards() {
    if (player_order.size() < 2) return;

    initializeDeck();

    // Deal reserve cards (3 per player)
    for (const auto& player_id : player_order) {
        PlayerGameState& ps = player_states[player_id];
        ps.reserve_cards.clear();
        for (int i = 0; i < 3 && !deck.empty(); i++) {
            ps.reserve_cards.push_back(deck.back());
            deck.pop_back();
        }
    }

    // Deal hand cards (3 per player)
    for (const auto& player_id : player_order) {
        PlayerGameState& ps = player_states[player_id];
        ps.hand.clear();
        for (int i = 0; i < 3 && !deck.empty(); i++) {
            ps.hand.push_back(deck.back());
            deck.pop_back();
        }
    }

    // Start discard pile
    if (!deck.empty()) {
        discard_pile.push_back(deck.back());
        deck.pop_back();
    }

    // Set first player's turn
    if (!player_order.empty()) {
        player_states[player_order[0]].is_turn = true;
    }
}

Card GambaGameState::getTopCard() const {
    if (!discard_pile.empty()) {
        return discard_pile.back();
    }
    return Card(Card::HEARTS, Card::ACE); // Default fallback
}

bool GambaGameState::canStartGame() const {
    return phase == GamePhase::WAITING;
}

void GambaGameState::pauseGame(const std::string& reason) {
    is_paused = true;
    paused_reason = reason;
}

void GambaGameState::resumeGame() {
    is_paused = false;
    paused_reason = "";
    disconnected_players.clear();
}

bool GambaGameState::canResumeGame() {
    return is_paused && disconnected_players.empty();
}