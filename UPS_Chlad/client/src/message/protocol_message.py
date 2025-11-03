class ProtocolMessage:
    """
    Represents a parsed protocol message.
    """
    
    def __init__(self, msg_type: int, player_id: str = "", room_id: str = "", data: dict = None):
        self.type = msg_type
        self.player_id = player_id
        self.room_id = room_id
        self.data = data or {}
    
    def to_dict(self) -> dict:
        """Convert to dictionary for easy access"""
        return {
            'type': self.type,
            'player_id': self.player_id,
            'room_id': self.room_id,
            **self.data  # Unpack data fields
        }
    
    def __repr__(self):
        return f"ProtocolMessage(type={self.type}, player={self.player_id}, room={self.room_id}, data={self.data})"
