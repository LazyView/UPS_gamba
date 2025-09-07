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

GambaGameState::PlayResult GambaGameState::playCards(const std::string& player_id, const std::vector<std::string>& cards) {
    if (phase != GamePhase::PLAYING) return PlayResult::GAME_OVER;

    // Check if it's this player's turn
    if (getCurrentPlayer() != player_id) {
        std::cout << "DEBUG: Not player's turn. Current: " << getCurrentPlayer() << ", Requested: " << player_id << std::endl;
        return PlayResult::INVALID_PLAYER;
    }

    if (cards.empty()) return PlayResult::INVALID_CARD;

    auto& player_state = player_states[player_id];  // Keep only this one
    std::cout << "DEBUG: Player " << player_id << " hand: " << player_state.getHandString() << std::endl;
    std::cout << "DEBUG: Trying to play cards: ";
    for (const auto& card : cards) std::cout << card << " ";
    std::cout << std::endl;

    std::vector<Card> cards_to_play;

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

        // If not in hand and hand is empty, check reserve cards
        if (!found && player_state.hand.empty() && !player_state.reserve_cards.empty()) {
            if (card_str == "RESERVE") {
                cards_to_play.push_back(player_state.reserve_cards[0]);
                found = true;
            }
        }

        if (!found) {
            std::cout << "DEBUG: Card " << card_str << " not found in hand" << std::endl;
            return PlayResult::INVALID_CARD;
        }
    }

    // Check if all cards have same value (for multiple card play)
    if (cards_to_play.size() > 1) {
        Card::Value first_value = cards_to_play[0].value;
        for (const auto& card : cards_to_play) {
            if (card.value != first_value) return PlayResult::INVALID_CARD;
        }
    }

    // Check if cards can be played
    Card top_card = getTopCard();
    if (!canPlayCard(cards_to_play[0], top_card)) {
        return PlayResult::PICKUP_REQUIRED;
    }

    // Execute the play
    for (const auto& card_to_remove : cards_to_play) {
        // Remove from hand
        auto it = std::find_if(player_state.hand.begin(), player_state.hand.end(),
            [&card_to_remove](const Card& c) {
                return c.value == card_to_remove.value && c.suit == card_to_remove.suit;
            });

        if (it != player_state.hand.end()) {
            player_state.hand.erase(it);
        } else {
            // Remove from reserve cards (for blind play)
            auto reserve_it = std::find_if(player_state.reserve_cards.begin(), player_state.reserve_cards.end(),
                [&card_to_remove](const Card& c) {
                    return c.value == card_to_remove.value && c.suit == card_to_remove.suit;
                });
            if (reserve_it != player_state.reserve_cards.end()) {
                player_state.reserve_cards.erase(reserve_it);
            }
        }

        // Add to discard pile
        discard_pile.push_back(card_to_remove);
    }

    // Handle special card effects
    handleSpecialCardEffects(cards_to_play[0]);

    // Draw cards back to hand (if deck not empty)
    while (!deck.empty() && player_state.hand.size() < 3) {
        player_state.hand.push_back(deck.back());
        deck.pop_back();
    }

    // Check win condition
    if (player_state.hasWon()) {
        phase = GamePhase::FINISHED;
        return PlayResult::SUCCESS;
    }

    // Move to next player
    nextPlayer();

    return PlayResult::SUCCESS;
}

bool GambaGameState::canPlayCard(const Card& card, const Card& top_card) const {
    // Special cards can always be played
    if (card.isSpecial()) return true;

    // Regular cards must be higher or equal value
    return static_cast<int>(card.value) >= static_cast<int>(top_card.value);
}

void GambaGameState::handleSpecialCardEffects(const Card& card) {
    switch (card.value) {
        case Card::TWO:
            // Wild card - no special effect, acts as joker
            break;

        case Card::SEVEN:
            // Reverse - next player must play 7 or lower
            // TODO: Implement reverse direction logic
            break;

        case Card::TEN:
            // Burn pile - remove discard pile from game
            discard_pile.clear();
            break;

        default:
            break;
    }
}

bool GambaGameState::pickupPile(const std::string& player_id) {
    if (phase != GamePhase::PLAYING) return false;

    if (getCurrentPlayer() != player_id) return false;

    auto& player_state = player_states[player_id];

    // Add all discard pile cards to player's hand
    for (const auto& card : discard_pile) {
        player_state.hand.push_back(card);
    }
    discard_pile.clear();

    // Move to next player
    nextPlayer();

    return true;
}