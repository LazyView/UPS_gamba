//
// Created by chlad on 9/11/2025.
//

#include "CardDeck.h"
#include <algorithm>
#include <random>
#include <stdexcept>
#include <sstream>

// Card methods
int Card::getValue() const {
    return static_cast<int>(rank);
}

std::string Card::toString() const {
    std::string suitStr;
    switch (suit) {
        case Suit::HEARTS: suitStr = "H"; break;
        case Suit::DIAMONDS: suitStr = "D"; break;
        case Suit::CLUBS: suitStr = "C"; break;
        case Suit::SPADES: suitStr = "S"; break;
    }

    std::string rankStr;
    switch (rank) {
        case Rank::ACE: rankStr = "A"; break;
        case Rank::JACK: rankStr = "J"; break;
        case Rank::QUEEN: rankStr = "Q"; break;
        case Rank::KING: rankStr = "K"; break;
        default: rankStr = std::to_string(static_cast<int>(rank)); break;
    }

    return rankStr + suitStr;
}

bool Card::isSpecial() const {
    return rank == Rank::TWO || rank == Rank::SEVEN || rank == Rank::TEN;
}

// CardDeck methods
CardDeck::CardDeck() {
    initializeStandardDeck();
}

void CardDeck::initializeStandardDeck() {
    cards.clear();

    // TESTING: Generate only 2s and Aces for quick testing
    for (int suit = 0; suit < 4; ++suit) {
        cards.emplace_back(static_cast<Suit>(suit), Rank::TWO);   // 2 of each suit
        cards.emplace_back(static_cast<Suit>(suit), Rank::ACE);   // Ace of each suit
    }
    
    // TODO: For full game, uncomment this:
    // for (int suit = 0; suit < 4; ++suit) {
    //     for (int rank = 2; rank <= 13; ++rank) {
    //         cards.emplace_back(static_cast<Suit>(suit), static_cast<Rank>(rank));
    //     }
    //     cards.emplace_back(static_cast<Suit>(suit), Rank::ACE);
    // }
}

void CardDeck::shuffle() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(cards.begin(), cards.end(), gen);
}

Card CardDeck::dealCard() {
    if (isEmpty()) {
        throw std::runtime_error("Cannot deal from empty deck");
    }

    Card card = cards.back();
    cards.pop_back();
    return card;
}

bool CardDeck::isEmpty() const {
    return cards.empty();
}

size_t CardDeck::size() const {
    return cards.size();
}

void CardDeck::clear() {
    cards.clear();
}

void CardDeck::addCards(const std::vector<Card>& cardsToAdd) {
    cards.insert(cards.end(), cardsToAdd.begin(), cardsToAdd.end());
}