"""
Player info widget - displays information about opponent.
"""

from PyQt5.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QLabel, 
    QGroupBox, QFrame
)
from PyQt5.QtCore import Qt
from PyQt5.QtGui import QFont


class PlayerInfoWidget(QWidget):
    """
    Widget for displaying opponent/player information.
    
    Shows:
    - Player name
    - Number of cards in hand
    - Number of reserve cards
    - Connection status
    - Turn indicator
    
    Responsibilities:
    - Display player state
    - Show visual indicators
    
    Does NOT:
    - Handle game logic
    - Manage connections
    """
    
    def __init__(self, parent=None):
        """
        Initialize player info widget.
        
        Args:
            parent: Parent widget
        """
        super().__init__(parent)
        
        # State
        self.player_name = ""
        self.hand_count = 0
        self.reserves_count = 0
        self.is_connected = True
        self.is_current_turn = False
        
        self._setup_ui()
    
    def _setup_ui(self):
        """Setup user interface"""
        layout = QVBoxLayout()
        layout.setContentsMargins(10, 10, 10, 10)
        layout.setSpacing(10)
        
        # Player name
        self.name_label = QLabel("Opponent: -")
        name_font = QFont()
        name_font.setPointSize(14)
        name_font.setBold(True)
        self.name_label.setFont(name_font)
        self.name_label.setStyleSheet("color: #2c3e50;")
        layout.addWidget(self.name_label)
        
        # Separator
        separator = QFrame()
        separator.setFrameShape(QFrame.HLine)
        separator.setFrameShadow(QFrame.Sunken)
        layout.addWidget(separator)
        
        # Cards info group
        cards_layout = QVBoxLayout()
        cards_layout.setSpacing(8)
        
        # Hand count
        hand_layout = QHBoxLayout()
        hand_icon = QLabel("🎴")
        hand_icon.setStyleSheet("font-size: 16px;")
        self.hand_label = QLabel("Cards in Hand: 0")
        self.hand_label.setStyleSheet("font-size: 12px; color: #34495e;")
        hand_layout.addWidget(hand_icon)
        hand_layout.addWidget(self.hand_label)
        hand_layout.addStretch()
        cards_layout.addLayout(hand_layout)
        
        # Reserve count
        reserve_layout = QHBoxLayout()
        reserve_icon = QLabel("📦")
        reserve_icon.setStyleSheet("font-size: 16px;")
        self.reserve_label = QLabel("Reserve Cards: 0")
        self.reserve_label.setStyleSheet("font-size: 12px; color: #34495e;")
        reserve_layout.addWidget(reserve_icon)
        reserve_layout.addWidget(self.reserve_label)
        reserve_layout.addStretch()
        cards_layout.addLayout(reserve_layout)
        
        layout.addLayout(cards_layout)
        
        # Status indicator
        status_layout = QHBoxLayout()
        self.status_icon = QLabel("✓")
        self.status_icon.setStyleSheet("font-size: 14px; color: #27ae60; font-weight: bold;")
        self.status_label = QLabel("Connected")
        self.status_label.setStyleSheet("font-size: 11px; color: #27ae60; font-style: italic;")
        status_layout.addWidget(self.status_icon)
        status_layout.addWidget(self.status_label)
        status_layout.addStretch()
        layout.addLayout(status_layout)
        
        # Turn indicator
        self.turn_indicator = QLabel("● Waiting")
        self.turn_indicator.setStyleSheet("""
            font-size: 12px;
            font-weight: bold;
            color: #95a5a6;
            padding: 5px;
            border-radius: 3px;
        """)
        layout.addWidget(self.turn_indicator)
        
        layout.addStretch()
        
        # Container with border
        self.setStyleSheet("""
            PlayerInfoWidget {
                background-color: #ecf0f1;
                border: 2px solid #bdc3c7;
                border-radius: 8px;
            }
        """)
        
        self.setLayout(layout)
        self.setFixedWidth(220)
    
    def update_info(self, name: str, hand_count: int, reserves_count: int):
        """
        Update player information.
        
        Args:
            name: Player name
            hand_count: Number of cards in hand
            reserves_count: Number of reserve cards
        """
        self.player_name = name
        self.hand_count = hand_count
        self.reserves_count = reserves_count
        
        # Update labels
        self.name_label.setText(f"Opponent: {name}")
        self.hand_label.setText(f"Cards in Hand: {hand_count}")
        self.reserve_label.setText(f"Reserve Cards: {reserves_count}")
    
    def set_connected(self, connected: bool):
        """
        Set connection status.
        
        Args:
            connected: True if player is connected
        """
        self.is_connected = connected
        
        if connected:
            self.status_icon.setText("✓")
            self.status_icon.setStyleSheet("font-size: 14px; color: #27ae60; font-weight: bold;")
            self.status_label.setText("Connected")
            self.status_label.setStyleSheet("font-size: 11px; color: #27ae60; font-style: italic;")
        else:
            self.status_icon.setText("✗")
            self.status_icon.setStyleSheet("font-size: 14px; color: #e74c3c; font-weight: bold;")
            self.status_label.setText("Disconnected")
            self.status_label.setStyleSheet("font-size: 11px; color: #e74c3c; font-style: italic;")
    
    def set_current_turn(self, is_turn: bool):
        """
        Set whether it's this player's turn.
        
        Args:
            is_turn: True if player's turn
        """
        self.is_current_turn = is_turn
        
        if is_turn:
            self.turn_indicator.setText("● Their Turn")
            self.turn_indicator.setStyleSheet("""
                font-size: 12px;
                font-weight: bold;
                color: #e67e22;
                background-color: #ffeaa7;
                padding: 5px;
                border-radius: 3px;
            """)
        else:
            self.turn_indicator.setText("● Waiting")
            self.turn_indicator.setStyleSheet("""
                font-size: 12px;
                font-weight: bold;
                color: #95a5a6;
                padding: 5px;
                border-radius: 3px;
            """)
    
    def reset(self):
        """Reset to initial state"""
        self.player_name = ""
        self.hand_count = 0
        self.reserves_count = 0
        self.is_connected = True
        self.is_current_turn = False
        
        self.name_label.setText("Opponent: -")
        self.hand_label.setText("Cards in Hand: 0")
        self.reserve_label.setText("Reserve Cards: 0")
        self.set_connected(True)
        self.set_current_turn(False)
