"""
Card representation and utilities for the Gamba card game.
"""

from typing import List, Optional
from utils.constants import (
    VALID_RANKS,
    VALID_SUITS,
    CARD_VALUES,
    CARD_VALUE_TWO,
    CARD_VALUE_SEVEN,
    CARD_VALUE_TEN,
    EMPTY_PILE_MARKER
)


class Card:
    """
    Represents a single playing card.
    
    Format: RANK + SUIT
    Examples: 2H, AS, 10D, KC
    
    Special cards:
    - 2 (Two): Wild card - can be played on anything
    - 7 (Seven): Forces next player to play ≤7
    - 10 (Ten): Burns the pile
    - 1S: Empty pile marker (not a real card)
    """
    
    def __init__(self, code: str):
        """
        Initialize card from code string.
        
        Args:
            code: Card code (e.g., "2H", "AS", "10D")
            
        Raises:
            ValueError: If code format is invalid
        """
        self.code = code.upper()
        
        # Parse rank and suit
        if len(code) < 2:
            raise ValueError(f"Invalid card code: {code}")
        
        self.suit = code[-1]  # Last character is suit
        self.rank = code[:-1]  # Everything else is rank
        
        # Validate
        if self.rank not in VALID_RANKS and code != EMPTY_PILE_MARKER:
            raise ValueError(f"Invalid rank: {self.rank}")
        if self.suit not in VALID_SUITS:
            raise ValueError(f"Invalid suit: {self.suit}")
        
        # Get numeric value
        self.value = CARD_VALUES.get(self.rank, 0)
    
    def is_special(self) -> bool:
        """Check if this is a special card (2, 7, or 10)"""
        return self.value in [CARD_VALUE_TWO, CARD_VALUE_SEVEN, CARD_VALUE_TEN]
    
    def is_wild(self) -> bool:
        """Check if this is a wild card (2)"""
        return self.value == CARD_VALUE_TWO
    
    def is_seven(self) -> bool:
        """Check if this is a seven (forces ≤7)"""
        return self.value == CARD_VALUE_SEVEN
    
    def is_ten(self) -> bool:
        """Check if this is a ten (burns pile)"""
        return self.value == CARD_VALUE_TEN
    
    def is_empty_marker(self) -> bool:
        """Check if this is the empty pile marker (1S)"""
        return self.code == EMPTY_PILE_MARKER
    
    def can_play_on(self, top_card: 'Card', must_play_low: bool = False) -> bool:
        """
        Check if this card can be played on top of another card.
        
        Args:
            top_card: The current top card on the pile
            must_play_low: If True, must play a card with value ≤7
            
        Returns:
            True if this card can be played
        """
        # Special case: empty pile (1S) - any card can be played
        if top_card.is_empty_marker():
            if must_play_low:
                return self.value <= CARD_VALUE_SEVEN or self.is_wild()
            return True
        
        # Special card: 2 (wild) can always be played
        if self.is_wild():
            return True

        if self.is_ten():
            return True
        
        # If must_play_low is active, only ≤7 cards (or wild) allowed
        if must_play_low:
            return self.value <= CARD_VALUE_SEVEN
        
        # Normal rule: play same or higher value
        # Special case: anything can be played on a 2
        if top_card.is_wild():
            return True
        
        # Regular play: same or higher value
        return self.value >= top_card.value
    
    def __eq__(self, other):
        """Check if two cards are equal"""
        if not isinstance(other, Card):
            return False
        return self.code == other.code
    
    def __hash__(self):
        """Make Card hashable (for use in sets/dicts)"""
        return hash(self.code)
    
    def __repr__(self):
        """String representation for debugging"""
        return f"Card({self.code})"
    
    def __str__(self):
        """String representation for display"""
        # Add suit symbols for better display
        suit_symbols = {
            'H': '♥',
            'D': '♦',
            'C': '♣',
            'S': '♠'
        }
        symbol = suit_symbols.get(self.suit, self.suit)
        return f"{self.rank}{symbol}"
    
    @staticmethod
    def parse_card_list(cards_str: str) -> List['Card']:
        """
        Parse a comma-separated list of card codes.
        
        Args:
            cards_str: Comma-separated card codes (e.g., "2H,AS,10D")
            
        Returns:
            List of Card objects
            
        Raises:
            ValueError: If any card code is invalid
        """
        if not cards_str or cards_str.strip() == "":
            return []
        
        card_codes = [code.strip() for code in cards_str.split(',')]
        # 1. Create the list of card objects
        card_list = [Card(code) for code in card_codes if code]
        
        # 2. Sort the list in-place using the card's 'value' attribute
        card_list.sort(key=lambda card: card.value)
        
        # 3. Return the now-sorted list
        return card_list
    
    @staticmethod
    def cards_to_string(cards: List['Card']) -> str:
        """
        Convert list of cards to comma-separated string.
        
        Args:
            cards: List of Card objects
            
        Returns:
            Comma-separated card codes (e.g., "2H,AS,10D")
        """
        return ','.join(card.code for card in cards)
    
    @staticmethod
    def validate_same_rank(cards: List['Card']) -> bool:
        """
        Check if all cards have the same rank.
        Required for playing multiple cards at once.
        
        Args:
            cards: List of Card objects
            
        Returns:
            True if all cards have the same rank
        """
        if not cards:
            return False
        if len(cards) == 1:
            return True
        
        first_rank = cards[0].rank
        return all(card.rank == first_rank for card in cards)
    
    @staticmethod
    def sort_by_value(cards: List['Card']) -> List['Card']:
        """
        Sort cards by value (ascending).
        
        Args:
            cards: List of Card objects
            
        Returns:
            Sorted list of cards
        """
        return sorted(cards, key=lambda c: c.value)


# Convenience function
def parse_cards(cards_str: str) -> List[Card]:
    """Parse card string into list of Card objects"""
    return Card.parse_card_list(cards_str)
