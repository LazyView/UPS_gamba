"""
Game log widget - displays game events and messages.
"""

from PyQt5.QtWidgets import QWidget, QVBoxLayout, QTextEdit, QLabel
from PyQt5.QtCore import Qt
from PyQt5.QtGui import QFont, QTextCursor
from datetime import datetime


class GameLogWidget(QWidget):
    """
    Widget for displaying game log messages.
    
    Shows:
    - Game events (card plays, pickups, etc.)
    - System messages
    - Player actions
    
    Responsibilities:
    - Display messages in chronological order
    - Auto-scroll to bottom
    - Format messages with timestamps
    
    Does NOT:
    - Generate messages (receives them)
    - Handle game logic
    """
    
    def __init__(self, parent=None):
        """
        Initialize game log widget.
        
        Args:
            parent: Parent widget
        """
        super().__init__(parent)
        
        self._setup_ui()
    
    def _setup_ui(self):
        """Setup user interface"""
        layout = QVBoxLayout()
        layout.setContentsMargins(5, 5, 5, 5)
        layout.setSpacing(5)
        
        # Title
        title = QLabel("Game Log")
        title.setStyleSheet("font-weight: bold; font-size: 12px; color: #2c3e50;")
        layout.addWidget(title)
        
        # Text area
        self.text_edit = QTextEdit()
        self.text_edit.setReadOnly(True)
        self.text_edit.setStyleSheet("""
            QTextEdit {
                background-color: #ffffff;
                border: 2px solid #bdc3c7;
                border-radius: 5px;
                padding: 5px;
                font-family: 'Courier New', monospace;
                font-size: 10px;
            }
        """)
        
        # Set maximum height
        self.text_edit.setMaximumHeight(200)
        
        layout.addWidget(self.text_edit)
        
        self.setLayout(layout)
        self.setFixedWidth(250)
    
    def add_message(self, message: str, message_type: str = "normal"):
        """
        Add a message to the log.
        
        Args:
            message: Message text
            message_type: Type of message ('normal', 'system', 'error', 'success')
        """
        # Get timestamp
        timestamp = datetime.now().strftime("%H:%M:%S")
        
        # Format based on type
        if message_type == "system":
            color = "#7f8c8d"  # Gray
            formatted = f"<span style='color: {color};'>[{timestamp}] {message}</span>"
        elif message_type == "error":
            color = "#e74c3c"  # Red
            formatted = f"<span style='color: {color}; font-weight: bold;'>[{timestamp}] {message}</span>"
        elif message_type == "success":
            color = "#27ae60"  # Green
            formatted = f"<span style='color: {color}; font-weight: bold;'>[{timestamp}] {message}</span>"
        elif message_type == "highlight":
            color = "#3498db"  # Blue
            formatted = f"<span style='color: {color}; font-weight: bold;'>[{timestamp}] {message}</span>"
        else:
            formatted = f"[{timestamp}] {message}"
        
        # Append to text edit
        self.text_edit.append(formatted)
        
        # Auto-scroll to bottom
        self.text_edit.moveCursor(QTextCursor.End)
    
    def add_game_event(self, event: str):
        """
        Add a game event message.
        
        Args:
            event: Event description
        """
        self.add_message(event, "normal")
    
    def add_player_action(self, player: str, action: str):
        """
        Add a player action message.
        
        Args:
            player: Player name
            action: Action description
        """
        self.add_message(f"{player}: {action}", "normal")
    
    def add_system_message(self, message: str):
        """
        Add a system message.
        
        Args:
            message: System message
        """
        self.add_message(message, "system")
    
    def add_error(self, message: str):
        """
        Add an error message.
        
        Args:
            message: Error message
        """
        self.add_message(message, "error")
    
    def add_success(self, message: str):
        """
        Add a success message.
        
        Args:
            message: Success message
        """
        self.add_message(message, "success")
    
    def clear(self):
        """Clear all messages"""
        self.text_edit.clear()
    
    def reset(self):
        """Reset log (alias for clear)"""
        self.clear()
