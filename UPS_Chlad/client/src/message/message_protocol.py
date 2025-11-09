from .protocol_message import ProtocolMessage


class MessageProtocol:
    """
    Handles parsing and building protocol messages with compact field codes.
    """

    # Field code mappings: old verbose field names → new compact codes
    FIELD_CODE_MAP = {
        # Field names
        "hand": "h",
        "reserves": "r",
        "opponent_hand": "oh",
        "opponent_reserves": "or",
        "opponent_name": "on",
        "top_card": "tc",
        "discard_pile_size": "dp",
        "deck_size": "dk",
        "must_play_low": "ml",
        "your_turn": "yt",
        "current_player": "cp",
        "status": "st",
        "name": "nm",
        "error": "er",
        "result": "rs",
        "cards": "cd",
        "winner": "wn",
        "reconnected_player": "rp",
        "disconnected_player": "dc",
        "broadcast_type": "bt",
        "joined_player": "jp",
        "players": "pl",
        "player_count": "pc",
        "room_full": "rf",
        "disconnect": "disc",
        "message": "msg",
        "reason": "rsn",

        # Status values
        "temporarily_disconnected": "temp",
        "reconnected": "recon",
        "success": "ok",
        "game_over": "end",
        "started": "start",
        "left": "lft",
        "timed_out": "tout",
        "invalid_message": "inv",

        # Result values
        "play_success": "pok",
        "pickup_success": "uok",
        "opponent_disconnect": "opdc",

        # Other common values
        "room_notification": "rnotif"
    }

    # Reverse mapping: compact codes → verbose field names
    REVERSE_FIELD_CODE_MAP = {v: k for k, v in FIELD_CODE_MAP.items()}

    @classmethod
    def get_compact_code(cls, field_name: str) -> str:
        """Convert full field name to compact code."""
        return cls.FIELD_CODE_MAP.get(field_name, field_name)

    @classmethod
    def get_full_field_name(cls, compact_code: str) -> str:
        """Convert compact code to full field name."""
        return cls.REVERSE_FIELD_CODE_MAP.get(compact_code, compact_code)

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
        
        # Parse key=value pairs (may be compact codes or full names)
        data = {}
        for i in range(3, len(parts)):
            if '=' in parts[i]:
                key, value = parts[i].split('=', 1)  # Split only on first =
                # Convert compact code to full field name (or keep as-is if already full)
                full_field_name = MessageProtocol.get_full_field_name(key)

                # ALSO convert compact value to full value (e.g., "temp" → "temporarily_disconnected")
                # BUT: Only if value is NOT a pure number (to avoid converting "1", "2", etc.)
                full_value = value
                if value and not value.lstrip('-').isdigit():
                    # Only convert if NOT numeric
                    full_value = MessageProtocol.get_full_field_name(value)

                data[full_field_name] = full_value

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

        # Add data fields (convert to compact codes for efficient transmission)
        if data:
            data_parts = []
            for key, value in data.items():
                compact_code = MessageProtocol.get_compact_code(key)
                # ALSO convert value if it's in the mapping
                compact_value = MessageProtocol.get_compact_code(str(value))
                data_parts.append(f"{compact_code}={compact_value}")
            message += "|".join(data_parts)

        # CRITICAL: Add newline terminator
        message += "\n"

        return message