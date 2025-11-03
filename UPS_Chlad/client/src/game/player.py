"""
Player state tracking for the Gamba card game.
"""

from typing import List, Optional
from .card import Card


class Player:
    """
    Represents a player in the game.
    Tracks hand, reserves, and connection status.
    """
    
    def __init__(self, name: str):
        """
        Initialize player.
        
        Args:
            name: Player's name
        """
        self.name = name
        self.hand: List[Card] = []
        self.reserves_count: int = 0
        self.is_current_player: bool = False
        self.is_connected: bool = True
    
    def set_hand(self, cards: List[Card]):
        """Set player's hand"""
        self.hand = cards.copy()
    
    def set_hand_from_string(self, cards_str: str):
        """
        Set hand from comma-separated card string.
        
        Args:
            cards_str: Card codes like "2H,AS,10D" or empty string
        """
        if cards_str and cards_str.strip():
            self.hand = Card.parse_card_list(cards_str)
        else:
            self.hand = []
    
    def add_cards_to_hand(self, cards: List[Card]):
        """Add cards to hand"""
        self.hand.extend(cards)
    
    def remove_cards_from_hand(self, cards: List[Card]) -> bool:
        """
        Remove cards from hand.
        
        Args:
            cards: Cards to remove
            
        Returns:
            True if all cards were found and removed
        """
        for card in cards:
            if card in self.hand:
                self.hand.remove(card)
            else:
                return False  # Card not in hand
        return True
    
    def has_card(self, card: Card) -> bool:
        """Check if player has a specific card"""
        return card in self.hand
    
    def has_cards(self, cards: List[Card]) -> bool:
        """Check if player has all specified cards"""
        return all(card in self.hand for card in cards)
    
    def get_hand_size(self) -> int:
        """Get number of cards in hand"""
        return len(self.hand)
    
    def clear_hand(self):
        """Remove all cards from hand"""
        self.hand.clear()
    
    def has_won(self) -> bool:
        """
        Check if player has won.
        Win condition: hand empty AND reserves empty
        """
        return len(self.hand) == 0 and self.reserves_count == 0
    
    def is_hand_empty(self) -> bool:
        """Check if hand is empty (may still have reserves)"""
        return len(self.hand) == 0
    
    def get_playable_cards(self, top_card: Card, must_play_low: bool = False) -> List[Card]:
        """
        Get list of cards from hand that can be played.
        
        Args:
            top_card: Current top card on pile
            must_play_low: If True, must play ≤7
            
        Returns:
            List of playable cards
        """
        return [card for card in self.hand if card.can_play_on(top_card, must_play_low)]
    
    def can_play_any_card(self, top_card: Card, must_play_low: bool = False) -> bool:
        """
        Check if player can play any card from hand.
        
        Args:
            top_card: Current top card on pile
            must_play_low: If True, must play ≤7
            
        Returns:
            True if at least one card can be played
        """
        return len(self.get_playable_cards(top_card, must_play_low)) > 0
    
    def to_dict(self) -> dict:
        """
        Convert player state to dictionary.
        
        Returns:
            Dictionary representation
        """
        return {
            'name': self.name,
            'hand': Card.cards_to_string(self.hand),
            'hand_size': len(self.hand),
            'reserves_count': self.reserves_count,
            'is_current_player': self.is_current_player,
            'is_connected': self.is_connected,
            'has_won': self.has_won()
        }
    
    def __repr__(self):
        return f"Player(name={self.name}, hand={len(self.hand)}, reserves={self.reserves_count}, connected={self.is_connected})"
    
    def __str__(self):
        status = "✓" if self.is_connected else "✗"
        turn = " (turn)" if self.is_current_player else ""
        return f"{status} {self.name}: {len(self.hand)} cards, {self.reserves_count} reserves{turn}"


class OpponentPlayer:
    """
    Represents opponent player (limited information).
    We don't know their hand, only hand size.
    """
    
    def __init__(self, name: str):
        """
        Initialize opponent.
        
        Args:
            name: Opponent's name
        """
        self.name = name
        self.hand_size: int = 0
        self.reserves_count: int = 0
        self.is_current_player: bool = False
        self.is_connected: bool = True
    
    def update_from_game_state(self, data: dict):
        """
        Update opponent info from GAME_STATE message data.
        
        Args:
            data: Dictionary from parsed GAME_STATE message
        """
        self.hand_size = int(data.get('opponent_hand', 0))
        self.reserves_count = int(data.get('opponent_reserves', 0))
        self.is_current_player = data.get('current_player') == self.name
        # Note: is_connected is updated separately via PLAYER_DISCONNECTED messages
    
    def has_won(self) -> bool:
        """Check if opponent has won"""
        return self.hand_size == 0 and self.reserves_count == 0
    
    def to_dict(self) -> dict:
        """Convert to dictionary"""
        return {
            'name': self.name,
            'hand_size': self.hand_size,
            'reserves_count': self.reserves_count,
            'is_current_player': self.is_current_player,
            'is_connected': self.is_connected,
            'has_won': self.has_won()
        }
    
    def __repr__(self):
        return f"OpponentPlayer(name={self.name}, hand_size={self.hand_size}, reserves={self.reserves_count}, connected={self.is_connected})"
    
    def __str__(self):
        status = "✓" if self.is_connected else "✗"
        turn = " (turn)" if self.is_current_player else ""
        return f"{status} {self.name}: {self.hand_size} cards, {self.reserves_count} reserves{turn}"
