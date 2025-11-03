from typing import Tuple
from enum import Enum

from .protocol_message import ProtocolMessage
from .message_types import ClientMessageType, ServerMessageType

class ValidationError(Enum):
    """Types of validation errors"""
    INVALID_FORMAT = "invalid_format"
    INVALID_STATE = "invalid_state"
    INVALID_GAME_RULE = "invalid_game_rule"
    INVALID_DATA = "invalid_data"


class MessageValidator:
    """
    Validates incoming messages.
    Tracks invalid message count for disconnect threshold.
    """
    
    def __init__(self, threshold: int = 3):
        self.invalid_count = 0
        self.threshold = threshold
    
    def validate_incoming(self, message: ProtocolMessage, current_state: str) -> Tuple[bool, str]:
        """
        Validate an incoming message from server.
        
        Args:
            message: Parsed message
            current_state: Current connection state (from ConnectionState enum)
            
        Returns:
            (is_valid, error_reason)
        """
        # Check message type is valid
        if not self._is_valid_server_message_type(message.type):
            return self._record_invalid("Invalid message type from server")
        
        # Check if message makes sense in current state
        if not self._is_valid_for_state(message.type, current_state):
            return self._record_invalid(f"Message type {message.type} invalid in state {current_state}")
        
        # Check required fields are present
        if not self._has_required_fields(message):
            return self._record_invalid("Missing required fields")
        
        return True, ""
    
    def validate_outgoing(self, message: ProtocolMessage, game_state) -> Tuple[bool, str]:
        """
        Validate an outgoing message before sending.
        
        This is CLIENT-SIDE validation before we even send to server.
        Prevents sending obviously invalid messages.
        
        Args:
            message: Message to send
            game_state: Current game state
            
        Returns:
            (is_valid, error_reason)
        """
        # Example: Can't play cards if not your turn
        if message.type == ClientMessageType.PLAY_CARDS:
            if not game_state.your_turn:
                return False, "Not your turn"
            
            # Validate cards exist in hand
            cards_str = message.data.get('cards', '')
            if cards_str != 'RESERVE':
                cards = cards_str.split(',')
                for card in cards:
                    if card not in [c.code for c in game_state.hand]:
                        return False, f"Card {card} not in hand"
        
        return True, ""
    
    def _record_invalid(self, reason: str) -> Tuple[bool, str]:
        """Record an invalid message and check threshold"""
        self.invalid_count += 1
        should_disconnect = self.invalid_count >= self.threshold
        
        if should_disconnect:
            reason += f" (threshold {self.threshold} reached)"
        
        return False, reason
    
    def reset(self):
        """Reset invalid count (e.g., after successful reconnect)"""
        self.invalid_count = 0
    
    def _is_valid_server_message_type(self, msg_type: int) -> bool:
        """Check if message type is a valid server message"""
        try:
            ServerMessageType(msg_type)
            return True
        except ValueError:
            return False
    
    def _is_valid_for_state(self, msg_type: int, current_state: str) -> bool:
        """
        Check if message type makes sense in current state.
        
        Example: Can't receive GAME_STATE if not in game.
        """
        # TODO: Implement state machine validation
        # This is where we check protocol state machine rules
        
        # Example rules:
        # - Can only receive CONNECTED after sending CONNECT
        # - Can only receive GAME_STATE when IN_GAME
        # - etc.
        
        return True  # For now, accept everything
    
    def _has_required_fields(self, message: ProtocolMessage) -> bool:
        """Check if message has required fields based on type"""
        # TODO: Implement required field checking per message type
        
        # Example:
        # - CONNECTED requires 'name' and 'status'
        # - GAME_STATE requires 'hand', 'top_card', etc.
        
        return True  # For now, accept everything