"""
Game rules validation for the Gamba card game.
Checks if moves are valid according to game rules.
"""

from typing import List, Tuple, Optional
from .card import Card
from .player import Player
from utils.constants import RESERVE_KEYWORD


class GameRules:
    """
    Validates game moves according to Gamba rules.
    
    Rules:
    - Can play same or higher value card (or special cards)
    - Can play multiple cards of same rank
    - After 7 is played, must play ≤7 next
    - 2 is wild (can play on anything)
    - 10 burns the pile
    - When hand is empty, play from reserves
    """
    
    @staticmethod
    def validate_card_play(
        player: Player,
        cards_to_play: List[Card],
        top_card: Card,
        must_play_low: bool = False
    ) -> Tuple[bool, str]:
        """
        Validate if player can play the specified cards.
        
        Args:
            player: The player attempting to play
            cards_to_play: Cards player wants to play
            top_card: Current top card on pile
            must_play_low: If True, must play ≤7
            
        Returns:
            (is_valid, error_reason) tuple
        """
        # Check if cards list is empty
        if not cards_to_play:
            return False, "No cards specified"
        
        # Check if player has all cards
        if not player.has_cards(cards_to_play):
            return False, "You don't have all these cards"
        
        # Check if all cards are same rank (when playing multiple)
        if len(cards_to_play) > 1:
            if not Card.validate_same_rank(cards_to_play):
                return False, "Can only play multiple cards of the same rank"
        
        # Check if cards can be played on top card
        # All cards must be playable (they're same rank, so check first one)
        first_card = cards_to_play[0]
        if not first_card.can_play_on(top_card, must_play_low):
            if must_play_low:
                return False, "Must play a card with value ≤7"
            else:
                return False, f"Cannot play {first_card.code} on {top_card.code}"
        
        return True, ""
    
    @staticmethod
    def validate_reserve_play(
        player: Player,
        top_card: Card,
        must_play_low: bool = False
    ) -> Tuple[bool, str]:
        """
        Validate if player can play from reserves.
        
        Args:
            player: The player
            top_card: Current top card on pile
            must_play_low: If True, must play ≤7
            
        Returns:
            (is_valid, error_reason) tuple
        """
        # Can only play reserves when hand is empty
        if not player.is_hand_empty():
            return False, "Can only play reserves when hand is empty"
        
        # Must have reserves left
        if player.reserves_count <= 0:
            return False, "No reserves left"
        
        # Note: We can't validate the actual reserve card
        # because the client doesn't know what it is until server reveals it
        return True, ""
    
    @staticmethod
    def validate_pickup_pile(
        player: Player,
        top_card: Card,
        must_play_low: bool = False
    ) -> Tuple[bool, str]:
        """
        Validate if player can (or must) pick up the pile.
        
        Args:
            player: The player
            top_card: Current top card on pile
            must_play_low: If True, must play ≤7
            
        Returns:
            (is_valid, error_reason) tuple
        """
        # Player picks up pile when they can't play any cards
        # This is always allowed (it's a fallback action)
        
        # But if player CAN play cards, they shouldn't pick up
        if player.can_play_any_card(top_card, must_play_low):
            return False, "You have cards that can be played"
        
        # If hand is empty, should try reserves first
        if player.is_hand_empty() and player.reserves_count > 0:
            return False, "Try playing from reserves first"
        
        return True, ""
    
    @staticmethod
    def can_start_game(player_count: int, max_players: int = 2) -> Tuple[bool, str]:
        """
        Check if game can be started.
        
        Args:
            player_count: Number of players in room
            max_players: Maximum players (default 2 for Gamba)
            
        Returns:
            (can_start, reason) tuple
        """
        if player_count < 2:
            return False, "Need at least 2 players to start"
        if player_count > max_players:
            return False, f"Room is full (max {max_players} players)"
        return True, ""
    
    @staticmethod
    def get_suggested_play(
        player: Player,
        top_card: Card,
        must_play_low: bool = False
    ) -> Optional[List[Card]]:
        """
        Suggest a valid play for the player.
        Returns the lowest valid card(s) to play.
        
        Args:
            player: The player
            top_card: Current top card on pile
            must_play_low: If True, must play ≤7
            
        Returns:
            List of suggested cards, or None if no valid play
        """
        playable = player.get_playable_cards(top_card, must_play_low)
        
        if not playable:
            return None
        
        # Sort by value and return lowest
        sorted_cards = Card.sort_by_value(playable)
        lowest = sorted_cards[0]
        
        # Check if we can play multiple of same rank
        same_rank = [c for c in playable if c.rank == lowest.rank]
        
        return same_rank
    
    @staticmethod
    def explain_move(
        cards: List[Card],
        top_card: Card,
        must_play_low: bool = False
    ) -> str:
        """
        Explain why a move is valid or invalid.
        
        Args:
            cards: Cards to play
            top_card: Current top card
            must_play_low: If True, must play ≤7
            
        Returns:
            Explanation string
        """
        if not cards:
            return "No cards to play"
        
        card = cards[0]
        
        if card.is_wild():
            return "2 is wild - can be played on anything"
        
        if card.is_ten():
            return "10 burns the pile"
        
        if card.is_seven():
            return "7 forces next player to play ≤7"
        
        if top_card.is_empty_marker():
            return "Pile is empty - any card can be played"
        
        if must_play_low:
            if card.value <= 7:
                return f"{card.code} is ≤7 (required after 7)"
            else:
                return f"{card.code} is too high (must be ≤7)"
        
        if top_card.is_wild():
            return "Anything can be played on a 2"
        
        if card.value >= top_card.value:
            return f"{card.code} ({card.value}) ≥ {top_card.code} ({top_card.value})"
        else:
            return f"{card.code} ({card.value}) < {top_card.code} ({top_card.value}) - too low"
