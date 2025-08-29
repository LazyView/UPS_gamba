//
// Created by chlad on 8/29/2025.
//
// Card representation
struct Card {
    enum Suit { HEARTS, DIAMONDS, CLUBS, SPADES };
    enum Value { ACE = 1, TWO = 2, THREE = 3, FOUR = 4, FIVE = 5, SIX = 6,
                SEVEN = 7, EIGHT = 8, NINE = 9, TEN = 10, JACK = 11, QUEEN = 12, KING = 13 };

    Suit suit;
    Value value;

    Card(Suit s, Value v) : suit(s), value(v) {}

    std::string toString() const {
        std::string result = std::to_string(static_cast<int>(value));
        switch(suit) {
            case HEARTS: result += "H"; break;
            case DIAMONDS: result += "D"; break;
            case CLUBS: result += "C"; break;
            case SPADES: result += "S"; break;
        }
        return result;
    }

    bool isSpecial() const {
        return value == TWO || value == SEVEN || value == TEN;
    }
};
