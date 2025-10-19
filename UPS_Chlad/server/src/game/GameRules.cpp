//
// Created by chlad on 9/11/2025.
//

#include "GameRules.h"
#include <algorithm>
#include <iostream>

bool GameRules::isValidPlay(const std::vector<Card>& cardsToPlay, const Card& topCard, bool mustPlaySevenOrLower) {
    if (cardsToPlay.empty()) {
        return false;
    }

    // Check if multiple cards are valid (must all be same rank)
    if (cardsToPlay.size() > 1 && !areMultipleCardsValid(cardsToPlay)) {
        return false;
    }

    // Check each card individually
    for (const Card& card : cardsToPlay) {
        if (!canPlayOn(card, topCard, mustPlaySevenOrLower)) {
            return false;
        }
    }

    return true;
}

bool GameRules::areMultipleCardsValid(const std::vector<Card>& cards) {
    if (cards.size() <= 1) {
        return true;
    }

    // All cards must have the same rank
    Rank firstRank = cards[0].rank;
    for (const Card& card : cards) {
        if (card.rank != firstRank) {
            return false;
        }
    }

    return true;
}

bool GameRules::canPlayOn(const Card& cardToPlay, const Card& topCard, bool mustPlaySevenOrLower) {
    
    // 2 is always a wild card - can be played on anything
    if (isWildCard(cardToPlay)) {
        return true;
    }
    
    // If top card is a 2 (wild card), any card can be played on it
    if (isWildCard(topCard)) {
        return true;
    }

    // If must play 7 or lower (after a 7 was played)
    if (mustPlaySevenOrLower) {
        bool result = cardToPlay.getValue() <= 7;
        return result;
    }

    // 10 can always be played (burn card)
    if (isBurnCard(cardToPlay)) {
        return true;
    }

    // Normal rule: card must be higher or equal value
    bool result = isHigherOrEqual(cardToPlay, topCard);
    return result;
}

bool GameRules::isHigherOrEqual(const Card& cardToPlay, const Card& topCard) {
    return cardToPlay.getValue() >= topCard.getValue();
}

bool GameRules::isWildCard(const Card& card) {
    return card.rank == Rank::TWO;
}

bool GameRules::isReverseCard(const Card& card) {
    return card.rank == Rank::SEVEN;
}

bool GameRules::isBurnCard(const Card& card) {
    return card.rank == Rank::TEN;
}

void GameRules::applySpecialCardEffects(const std::vector<Card>& cardsPlayed,
                                      std::vector<Card>& discardPile,
                                      bool& clockwise,
                                      bool& mustPlaySevenOrLower) {
    // Reset special states first
    mustPlaySevenOrLower = false;

    for (const Card& card : cardsPlayed) {
        if (isReverseCard(card)) {
            // 7 - Reverse Direction: Next player must play 7 or lower
            mustPlaySevenOrLower = true;
        }

        if (isBurnCard(card)) {
            // 10 - Burn Card: Remove entire discard pile from game
            discardPile.clear();
            // Note: The current played cards are added to discard pile after this function
            // so the 10 will be the only card in the discard pile
        }
    }

    // Note: Wild cards (2s) don't have special effects beyond being playable on anything
    // Direction reversal could be implemented here if needed for the game variant
}

bool GameRules::canPlayTogether(const std::vector<Card>& cards) {
    return areMultipleCardsValid(cards);
}

int GameRules::getEffectiveValue(const Card& card) {
    // For most game logic, use the card's actual value
    // Special cards are handled in their respective functions
    return card.getValue();
}