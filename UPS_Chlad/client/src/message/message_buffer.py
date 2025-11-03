class MessageBuffer:
    """
    Accumulates TCP data and extracts complete messages.
    Messages are delimited by newline (\n).
    """
    
    MAX_BUFFER_SIZE = 1024 * 1024  # 1MB - protect against memory issues
    
    def __init__(self, max_size: int = None):
        self._buffer = ""
        self.max_size = max_size or self.MAX_BUFFER_SIZE
    
    def add_data(self, data: str) -> list[str]:
        """
        Add received data to buffer and return list of complete messages.
        
        Args:
            data: Raw string data from socket
            
        Returns:
            List of complete messages (without \n)
            
        Example:
            buffer.add_data("100|Alice||name=")  # Returns []
            buffer.add_data("Alice\n101|Bob")    # Returns ["100|Alice||name=Alice"]
            buffer.add_data("||name=Bob\n")      # Returns ["101|Bob||name=Bob"]
        """
        self._buffer += data
        
        # Check buffer size limit
        if len(self._buffer) > self.max_size:
            raise ValueError(f"Buffer overflow: size {len(self._buffer)} exceeds max {self.max_size}")
        
        messages = []
        
        # Extract all complete messages (ending with \n)
        while '\n' in self._buffer:
            idx = self._buffer.index('\n')
            message = self._buffer[:idx]
            self._buffer = self._buffer[idx + 1:]
            
            if message:  # Ignore empty lines
                messages.append(message)
        
        return messages
    
    def clear(self):
        """Clear buffer (e.g., after disconnect)"""
        self._buffer = ""
    
    def has_data(self) -> bool:
        """Check if buffer has incomplete data"""
        return len(self._buffer) > 0