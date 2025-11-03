"""
Game state management for the Gamba card game.
Tracks current game state with serialization for reconnection.
"""

from typing import Optional, Dict, Any
from datetime import datetime

from .card import Card
from .player import Player, OpponentPlayer
from utils.constants import EMPTY_PILE_MARKER
from utils import constants


class GameState:
    """
    Maintains complete game state.
    Can serialize/restore for reconnection.
    """
    
    def __init__(self):
        """Initialize empty game state"""
        # Connection info
        self.player_name: str = ""
        self.room_id: str = ""
        
        # Players
        self.player: Optional[Player] = None
        self.opponent: Optional[OpponentPlayer] = None
        
        # Game state
        self.top_card: Optional[Card] = None
        self.must_play_low: bool = False
        self.current_player_name: str = ""
        self.deck_size: int = 0
        self.discard_pile_size: int = 0
        
        # Meta state
        self.last_update: Optional[datetime] = None
        self.connection_state: str = constants.STATE_DISCONNECTED
        self.in_game: bool = False
    
    def initialize_player(self, name: str):
        """
        Initialize the player.
        
        Args:
            name: Player's name
        """
        self.player_name = name
        self.player = Player(name)
    
    def update_from_game_state_message(self, data: Dict[str, str]):
        """
        Update state from GAME_STATE message (type 106).
        
        Args:
            data: Parsed message data dictionary
        """
        # Ensure player exists
        if not self.player:
            self.initialize_player(data.get('player_id', 'Unknown'))
        
        # Update player's hand
        hand_str = data.get('hand', '')
        self.player.set_hand_from_string(hand_str)
        
        # Update reserves
        self.player.reserves_count = int(data.get('reserves', 0))
        
        # Update current player
        self.current_player_name = data.get('current_player', '')
        self.player.is_current_player = (self.current_player_name == self.player_name)
        
        # Update top card
        top_card_str = data.get('top_card', EMPTY_PILE_MARKER)
        try:
            self.top_card = Card(top_card_str)
        except ValueError:
            # Invalid card, use empty marker
            self.top_card = Card(EMPTY_PILE_MARKER)
        
        # Update must_play_low flag
        self.must_play_low = data.get('must_play_low', 'false').lower() == 'true'
        
        # Update pile info
        self.deck_size = int(data.get('deck_size', 0))
        self.discard_pile_size = int(data.get('discard_pile_size', 0))
        
        # Update opponent
        opponent_name = data.get('opponent_name', '')
        if opponent_name:
            if not self.opponent or self.opponent.name != opponent_name:
                self.opponent = OpponentPlayer(opponent_name)
            self.opponent.update_from_game_state(data)
            self.opponent.is_current_player = (self.current_player_name == opponent_name)
        
        # Update meta
        self.last_update = datetime.now()
        self.in_game = True
    
    def your_turn(self) -> bool:
        """Check if it's your turn"""
        return self.player and self.player.is_current_player
    
    def opponent_turn(self) -> bool:
        """Check if it's opponent's turn"""
        return self.opponent and self.opponent.is_current_player
    
    def mark_opponent_disconnected(self):
        """Mark opponent as disconnected"""
        if self.opponent:
            self.opponent.is_connected = False
    
    def mark_opponent_reconnected(self):
        """Mark opponent as reconnected"""
        if self.opponent:
            self.opponent.is_connected = True
    
    def reset_game(self):
        """Reset game state (after game over)"""
        if self.player:
            self.player.clear_hand()
            self.player.reserves_count = 0
            self.player.is_current_player = False
        
        self.opponent = None
        self.top_card = None
        self.must_play_low = False
        self.current_player_name = ""
        self.deck_size = 0
        self.discard_pile_size = 0
        self.in_game = False
        self.room_id = ""
    
    def reset_connection(self):
        """Reset connection state (full disconnect)"""
        self.reset_game()
        self.player = None
        self.player_name = ""
        self.connection_state = constants.STATE_DISCONNECTED
    
    # ========================================================================
    # SERIALIZATION (for reconnection)
    # ========================================================================
    
    def serialize(self) -> Dict[str, Any]:
        """
        Serialize state for persistence/reconnection.
        
        Returns:
            Dictionary representation of state
        """
        return {
            # Connection
            'player_name': self.player_name,
            'room_id': self.room_id,
            'connection_state': self.connection_state,
            'in_game': self.in_game,
            
            # Player
            'player': self.player.to_dict() if self.player else None,
            
            # Opponent
            'opponent': self.opponent.to_dict() if self.opponent else None,
            
            # Game state
            'top_card': self.top_card.code if self.top_card else None,
            'must_play_low': self.must_play_low,
            'current_player_name': self.current_player_name,
            'deck_size': self.deck_size,
            'discard_pile_size': self.discard_pile_size,
            
            # Meta
            'last_update': self.last_update.isoformat() if self.last_update else None
        }
    
    def restore(self, data: Dict[str, Any]):
        """
        Restore state from serialized data.
        
        Args:
            data: Dictionary from serialize()
        """
        # Connection
        self.player_name = data.get('player_name', '')
        self.room_id = data.get('room_id', '')
        self.connection_state = data.get('connection_state', constants.STATE_DISCONNECTED)
        self.in_game = data.get('in_game', False)
        
        # Player
        player_data = data.get('player')
        if player_data:
            self.player = Player(player_data['name'])
            self.player.set_hand_from_string(player_data.get('hand', ''))
            self.player.reserves_count = player_data.get('reserves_count', 0)
            self.player.is_current_player = player_data.get('is_current_player', False)
            self.player.is_connected = player_data.get('is_connected', True)
        
        # Opponent
        opponent_data = data.get('opponent')
        if opponent_data:
            self.opponent = OpponentPlayer(opponent_data['name'])
            self.opponent.hand_size = opponent_data.get('hand_size', 0)
            self.opponent.reserves_count = opponent_data.get('reserves_count', 0)
            self.opponent.is_current_player = opponent_data.get('is_current_player', False)
            self.opponent.is_connected = opponent_data.get('is_connected', True)
        
        # Game state
        top_card_code = data.get('top_card')
        if top_card_code:
            try:
                self.top_card = Card(top_card_code)
            except ValueError:
                self.top_card = None
        
        self.must_play_low = data.get('must_play_low', False)
        self.current_player_name = data.get('current_player_name', '')
        self.deck_size = data.get('deck_size', 0)
        self.discard_pile_size = data.get('discard_pile_size', 0)
        
        # Meta
        last_update_str = data.get('last_update')
        if last_update_str:
            try:
                self.last_update = datetime.fromisoformat(last_update_str)
            except ValueError:
                self.last_update = None
    
    # ========================================================================
    # UTILITY METHODS
    # ========================================================================
    
    def get_game_info(self) -> str:
        """
        Get human-readable game info string.
        
        Returns:
            Game info summary
        """
        if not self.in_game:
            return "Not in game"
        
        info = []
        
        # Your info
        if self.player:
            info.append(f"You: {len(self.player.hand)} cards, {self.player.reserves_count} reserves")
        
        # Opponent info
        if self.opponent:
            status = "disconnected" if not self.opponent.is_connected else "connected"
            info.append(f"Opponent ({self.opponent.name}): {self.opponent.hand_size} cards, {self.opponent.reserves_count} reserves ({status})")
        
        # Top card
        if self.top_card:
            info.append(f"Top card: {self.top_card}")
        
        # Must play low
        if self.must_play_low:
            info.append("⚠️ Must play ≤7")
        
        # Turn
        if self.your_turn():
            info.append("🎯 YOUR TURN")
        elif self.opponent_turn():
            info.append("⏳ Opponent's turn")
        
        return " | ".join(info)
    
    def to_dict(self) -> Dict[str, Any]:
        """
        Convert to dictionary for display/debugging.
        
        Returns:
            Dictionary representation
        """
        return {
            'player_name': self.player_name,
            'room_id': self.room_id,
            'connection_state': self.connection_state,
            'in_game': self.in_game,
            'your_turn': self.your_turn(),
            'player': self.player.to_dict() if self.player else None,
            'opponent': self.opponent.to_dict() if self.opponent else None,
            'top_card': str(self.top_card) if self.top_card else None,
            'must_play_low': self.must_play_low,
            'current_player': self.current_player_name,
            'deck_size': self.deck_size,
            'discard_pile_size': self.discard_pile_size
        }
    
    def __repr__(self):
        return f"GameState(player={self.player_name}, room={self.room_id}, in_game={self.in_game}, your_turn={self.your_turn()})"
