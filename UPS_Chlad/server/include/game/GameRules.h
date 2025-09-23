//
// Created by chlad on 9/11/2025.
//

#ifndef GAMERULES_H
#define GAMERULES_H

#include "CardDeck.h"
#include <vector>

class GameRules {
public:
    // Core game rule validation
    static bool isValidPlay(const std::vector<Card>& cardsToPlay, const Card& topCard, bool mustPlaySevenOrLower);

    // Check if cards can be played together (same rank)
    static bool canPlayTogether(const std::vector<Card>& cards);

    // Check if a single card can be played on top of another
    static bool canPlayOn(const Card& cardToPlay, const Card& topCard, bool mustPlaySevenOrLower);

    // Check if card is higher or equal value
    static bool isHigherOrEqual(const Card& cardToPlay, const Card& topCard);

    // Special card handling
    static bool isWildCard(const Card& card);        // 2 - Wild Card
    static bool isReverseCard(const Card& card);     // 7 - Reverse Direction
    static bool isBurnCard(const Card& card);        // 10 - Burn Card

    // Apply special card effects
    static void applySpecialCardEffects(const std::vector<Card>& cardsPlayed,
                                      std::vector<Card>& discardPile,
                                      bool& clockwise,
                                      bool& mustPlaySevenOrLower);

    // Check if multiple cards are valid (all same rank)
    static bool areMultipleCardsValid(const std::vector<Card>& cards);

    // Get the effective value of a card (considering special rules)
    static int getEffectiveValue(const Card& card);
};

#endif //GAMERULES_H