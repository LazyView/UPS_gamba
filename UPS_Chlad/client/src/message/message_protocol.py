from .protocol_message import ProtocolMessage


class MessageProtocol:
    """
    Handles parsing and building protocol messages.
    """
    
    @staticmethod
    def parse(message: str) -> ProtocolMessage:
        """
        Parse a raw message string into ProtocolMessage.
        
        Args:
            message: Raw message without \n (e.g., "100|Alice||name=Alice|status=success")
            
        Returns:
            ProtocolMessage object
            
        Raises:
            ValueError: If message format is invalid
        """
        parts = message.split('|')
        
        if len(parts) < 3:
            raise ValueError(f"Invalid message format: {message}")
        
        try:
            msg_type = int(parts[0])
        except ValueError:
            raise ValueError(f"Invalid message type: {parts[0]}")
        
        player_id = parts[1]
        room_id = parts[2]
        
        # Parse key=value pairs
        data = {}
        for i in range(3, len(parts)):
            if '=' in parts[i]:
                key, value = parts[i].split('=', 1)  # Split only on first =
                data[key] = value
        
        return ProtocolMessage(msg_type, player_id, room_id, data)
    
    @staticmethod
    def build(msg_type: int, player_id: str = "", room_id: str = "", **data) -> str:
        """
        Build a message string from components.
        
        Args:
            msg_type: Message type code
            player_id: Player identifier (empty string if not needed)
            room_id: Room identifier (empty string if not needed)
            **data: Key-value pairs for message data
            
        Returns:
            Complete message string WITH \n terminator
            
        Example:
            build(0, player_id="", room_id="", name="Alice")
            → "0|||name=Alice\n"
        """
        message = f"{msg_type}|{player_id}|{room_id}|"
        
        # Add data fields
        if data:
            data_parts = [f"{key}={value}" for key, value in data.items()]
            message += "|".join(data_parts)
        
        # CRITICAL: Add newline terminator
        message += "\n"
        
        return message