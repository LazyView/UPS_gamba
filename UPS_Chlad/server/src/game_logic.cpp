// game_logic.cpp - Gamba Card Game Logic Implementation
// KIV/UPS Network Programming Project

#include "../include/game_logic.h"
#include <iostream>

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
    for (int suit = 0; suit < 1; suit++) {
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

    // If discard pile is empty (after burn or pickup), return a special "no card" indicator
    // using a special card that indicates "any card can be played"
    return Card(Card::HEARTS, static_cast<Card::Value>(0)); // Value 0 = "no restrictions"
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

// Fixed playCards() method with corrected reserve card handling

GambaGameState::PlayResult GambaGameState::playCards(const std::string& player_id, const std::vector<std::string>& cards) {
    if (phase != GamePhase::PLAYING) return PlayResult::GAME_OVER;

    // Check if it's this player's turn
    if (getCurrentPlayer() != player_id) {
        std::cout << "DEBUG: Not player's turn. Current: " << getCurrentPlayer() << ", Requested: " << player_id << std::endl;
        return PlayResult::INVALID_PLAYER;
    }

    if (cards.empty()) return PlayResult::INVALID_CARD;

    auto& player_state = player_states[player_id];
    std::cout << "DEBUG: Player " << player_id << " hand: " << player_state.getHandString() << std::endl;
    std::cout << "DEBUG: Reserve cards: " << player_state.reserve_cards.size() << std::endl;

    std::vector<Card> cards_to_play;
    std::vector<Card> reserve_cards_revealed; // Track revealed reserve cards

    // Parse and validate cards
    for (const std::string& card_str : cards) {
        bool found = false;

        // Check in hand first
        for (auto& card : player_state.hand) {
            if (card.toString() == card_str) {
                cards_to_play.push_back(card);
                found = true;
                break;
            }
        }

        // FIXED: Reserve card handling - remove BEFORE validation
        if (!found && player_state.hand.empty() && !player_state.reserve_cards.empty()) {
            if (card_str == "RESERVE") {
                // Remove reserve card IMMEDIATELY (before validation)
                Card revealed_card = player_state.reserve_cards[0];
                player_state.reserve_cards.erase(player_state.reserve_cards.begin());

                std::cout << "DEBUG: Reserve card revealed: " << revealed_card.toString() << std::endl;
                std::cout << "DEBUG: Reserve cards remaining: " << player_state.reserve_cards.size() << std::endl;

                cards_to_play.push_back(revealed_card);
                reserve_cards_revealed.push_back(revealed_card);
                found = true;
            }
        }

        if (!found) {
            std::cout << "DEBUG: Card " << card_str << " not found" << std::endl;
            return PlayResult::INVALID_CARD;
        }
    }

    // Check if cards can be played
    Card top_card = getTopCard();
    if (!canPlayCard(cards_to_play[0], top_card)) {
        std::cout << "DEBUG: Cannot play card " << cards_to_play[0].toString() << " on " << top_card.toString() << std::endl;

        // CRITICAL: If this was a reserve card, it's now revealed and lost forever
        // Add it to the discard pile so it can be picked up
        if (!reserve_cards_revealed.empty()) {
            std::cout << "DEBUG: Invalid reserve card " << reserve_cards_revealed[0].toString() << " added to discard pile" << std::endl;
            discard_pile.push_back(reserve_cards_revealed[0]);
        }

        return PlayResult::PICKUP_REQUIRED;
    }

    // Execute the play - only need to handle hand cards now since reserve already removed
    for (const auto& card_to_remove : cards_to_play) {

        // Only try to remove from hand (reserve cards already removed above)
        if (reserve_cards_revealed.empty()) { // This was a hand card
            auto it = std::find_if(player_state.hand.begin(), player_state.hand.end(),
                [&card_to_remove](const Card& c) {
                    return c.value == card_to_remove.value && c.suit == card_to_remove.suit;
                });

            if (it != player_state.hand.end()) {
                player_state.hand.erase(it);
            }
        }

        // Add to discard pile (for both hand cards and reserve cards)
        discard_pile.push_back(card_to_remove);
    }

    // Handle special card effects
    handleSpecialCardEffects(cards_to_play[0]);

    // Check if deck is empty and transition to Phase 2
    if (deck.empty() && !deck_empty_phase) {
        deck_empty_phase = true;
        std::cout << "DEBUG: Deck is now empty - entering Phase 2 (no more drawing)" << std::endl;
    }

    // Draw cards back to hand ONLY if deck is not empty
    if (!deck_empty_phase) {
        while (!deck.empty() && player_state.hand.size() < 3) {
            player_state.hand.push_back(deck.back());
            deck.pop_back();

            // Check if this was the last card
            if (deck.empty()) {
                deck_empty_phase = true;
                std::cout << "DEBUG: Deck exhausted during drawing - Phase 2 begins" << std::endl;
                break;
            }
        }
    }

	// Check win condition
    if (player_state.hasWon()) {
        phase = GamePhase::FINISHED;

        // NEW: Broadcast game over to all players
        std::cout << "DEBUG: Player " << player_id << " has won the game!" << std::endl;

        return PlayResult::SUCCESS;
    }

    // Move to next player
    nextPlayer();

    return PlayResult::SUCCESS;
}

bool GambaGameState::canPlayCard(const Card& card, const Card& top_card) const {
    // Special cards can always be played
    if (card.isSpecial()) return true;

    // If top card has value 0, it means discard pile is empty - any card can be played
    if (static_cast<int>(top_card.value) == 0) {
        return true;
    }

    // If reverse direction is active (after a 7), only 7 or lower can be played
    if (reverse_direction_active) {
        return static_cast<int>(card.value) <= 7;
    }

    // Regular cards must be higher or equal value
    return static_cast<int>(card.value) >= static_cast<int>(top_card.value);
}

void GambaGameState::handleSpecialCardEffects(const Card& card) {
    switch (card.value) {
        case Card::TWO:
            // Wild card - acts as joker, can be played on anything
            reverse_direction_active = false;
            break;

        case Card::SEVEN:
            // Reverse direction - next player must play 7 or lower
            reverse_direction_active = true;
            std::cout << "DEBUG: Seven played - reverse direction activated" << std::endl;
            break;

        case Card::TEN:
            // Burn pile - remove entire discard pile from game
            std::cout << "DEBUG: Ten played - burning discard pile (size: " << discard_pile.size() << ")" << std::endl;
            discard_pile.clear();
            reverse_direction_active = false;
            std::cout << "DEBUG: Discard pile burned - next player can play any card" << std::endl;
            break;

        default:
            // Regular card played - clear reverse direction if it was active and card is valid
            if (reverse_direction_active && static_cast<int>(card.value) <= 7) {
                reverse_direction_active = false;
                std::cout << "DEBUG: Reverse direction cleared by playing " << card.toString() << std::endl;
            }
            break;
    }
}

bool GambaGameState::pickupPile(const std::string& player_id) {
    if (phase != GamePhase::PLAYING) return false;

    if (getCurrentPlayer() != player_id) return false;

    auto& player_state = player_states[player_id];

    std::cout << "DEBUG: Player picking up pile (size: " << discard_pile.size() << ")" << std::endl;

    // Add all discard pile cards to player's hand
    for (const auto& card : discard_pile) {
        player_state.hand.push_back(card);
    }
    discard_pile.clear();

    std::cout << "DEBUG: Pile picked up - player now has " << player_state.hand.size() << " cards" << std::endl;

    // Move to next player
    nextPlayer();

    return true;
}