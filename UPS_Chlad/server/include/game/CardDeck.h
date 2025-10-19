//
// Created by chlad on 9/11/2025.
//

#ifndef CARDDECK_H
#define CARDDECK_H

#include <vector>
#include <string>

enum class Suit {
    HEARTS, DIAMONDS, CLUBS, SPADES
};

enum class Rank {
    TWO = 2, THREE = 3, FOUR = 4, FIVE = 5, SIX = 6, SEVEN = 7,
    EIGHT = 8, NINE = 9, TEN = 10, JACK = 11, QUEEN = 12, KING = 13, ACE = 14
};

struct Card {
    Suit suit;
    Rank rank;

    Card(Suit s, Rank r) : suit(s), rank(r) {}

    // Get rank value for comparison
    int getValue() const;

    // Get string representation
    std::string toString() const;

    // Check if card is special
    bool isSpecial() const;
};

class CardDeck {
private:
    std::vector<Card> cards;

public:
    CardDeck();

    // Initialize with standard 52-card deck
    void initializeStandardDeck();

    // Shuffle the deck
    void shuffle();

    // Deal a card (removes from deck)
    Card dealCard();

    // Check if deck is empty
    bool isEmpty() const;

    // Get remaining card count
    size_t size() const;

    // Clear the deck
    void clear();

    // Add cards back to deck (for recycling discard pile)
    void addCards(const std::vector<Card>& cardsToAdd);
};

#endif //CARDDECK_H