"""
Lobby widget - displayed after joining a room, waiting for game to start.
"""

from PyQt5.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QLabel, 
    QPushButton, QGroupBox, QListWidget, QFrame
)
from PyQt5.QtCore import Qt, pyqtSignal
from PyQt5.QtGui import QFont

from utils import get_logger


class LobbyWidget(QWidget):
    """
    Lobby screen shown after joining a room.
    
    Displays:
    - Room information
    - List of players in room
    - Start game button (when ready)
    
    Emits:
    - start_game_requested: When user clicks Start Game
    - leave_room_requested: When user wants to leave
    
    Responsibilities:
    - Display room state
    - Allow user to start game
    - Show player list
    
    Does NOT:
    - Handle network operations
    - Validate game state
    """
    
    # Signals
    start_game_requested = pyqtSignal()
    leave_room_requested = pyqtSignal()
    
    def __init__(self, parent=None):
        """
        Initialize lobby widget.
        
        Args:
            parent: Parent widget
        """
        super().__init__(parent)
        
        self.logger = get_logger()
        
        # State
        self.room_id = ""
        self.player_count = 0
        self.max_players = 2
        self.players = []
        self.room_full = False
        
        self._setup_ui()
        
        self.logger.info("LobbyWidget initialized")
    
    def _setup_ui(self):
        """Setup user interface"""
        layout = QVBoxLayout()
        layout.setContentsMargins(20, 20, 20, 20)
        layout.setSpacing(15)
        
        # Title
        title = QLabel("Game Lobby")
        title.setAlignment(Qt.AlignCenter)
        title_font = QFont()
        title_font.setPointSize(24)
        title_font.setBold(True)
        title.setFont(title_font)
        layout.addWidget(title)
        
        # Room info group
        room_group = QGroupBox("Room Information")
        room_layout = QVBoxLayout()
        
        self.room_id_label = QLabel("Room: -")
        self.room_id_label.setStyleSheet("font-size: 14px;")
        room_layout.addWidget(self.room_id_label)
        
        self.player_count_label = QLabel("Players: 0/2")
        self.player_count_label.setStyleSheet("font-size: 14px;")
        room_layout.addWidget(self.player_count_label)
        
        room_group.setLayout(room_layout)
        layout.addWidget(room_group)
        
        # Players list group
        players_group = QGroupBox("Players")
        players_layout = QVBoxLayout()
        
        self.players_list = QListWidget()
        self.players_list.setStyleSheet("""
            QListWidget {
                font-size: 14px;
                padding: 5px;
            }
            QListWidget::item {
                padding: 8px;
                border-bottom: 1px solid #e0e0e0;
            }
        """)
        self.players_list.setMaximumHeight(150)
        players_layout.addWidget(self.players_list)
        
        players_group.setLayout(players_layout)
        layout.addWidget(players_group)
        
        # Status label
        self.status_label = QLabel("Waiting for players...")
        self.status_label.setAlignment(Qt.AlignCenter)
        self.status_label.setStyleSheet("""
            QLabel {
                font-size: 16px;
                font-style: italic;
                color: #666;
                padding: 10px;
            }
        """)
        layout.addWidget(self.status_label)
        
        # Buttons
        button_layout = QHBoxLayout()
        button_layout.addStretch()
        
        self.start_button = QPushButton("Start Game")
        self.start_button.setEnabled(False)
        self.start_button.clicked.connect(self._on_start_clicked)
        self.start_button.setMinimumWidth(150)
        self.start_button.setMinimumHeight(40)
        self.start_button.setStyleSheet("""
            QPushButton {
                background-color: #4CAF50;
                color: white;
                font-size: 16px;
                font-weight: bold;
                border-radius: 5px;
                padding: 10px;
            }
            QPushButton:hover {
                background-color: #45a049;
            }
            QPushButton:disabled {
                background-color: #cccccc;
                color: #666666;
            }
        """)
        button_layout.addWidget(self.start_button)
        
        button_layout.addStretch()
        layout.addLayout(button_layout)
        
        # Add stretch at bottom
        layout.addStretch()
        
        self.setLayout(layout)
    
    def update_room_info(self, room_id: str, player_count: int, players: list, room_full: bool):
        """
        Update room information.
        
        Args:
            room_id: Room identifier
            player_count: Number of players in room
            players: List of player names
            room_full: Whether room is full
        """
        self.room_id = room_id
        self.player_count = player_count
        self.players = players
        self.room_full = room_full
        
        # Update labels
        self.room_id_label.setText(f"Room: {room_id}")
        self.player_count_label.setText(f"Players: {player_count}/{self.max_players}")
        
        # Update players list
        self.players_list.clear()
        for i, player in enumerate(players, 1):
            self.players_list.addItem(f"{i}. {player}")
        
        # Update status and button
        if room_full:
            self.status_label.setText("Room is full! Ready to start.")
            self.status_label.setStyleSheet("""
                QLabel {
                    font-size: 16px;
                    font-weight: bold;
                    color: #4CAF50;
                    padding: 10px;
                }
            """)
            self.start_button.setEnabled(True)
            self.start_button.setText("Start game")
        else:
            self.status_label.setText(f"Waiting for players... ({player_count}/{self.max_players})")
            self.status_label.setStyleSheet("""
                QLabel {
                    font-size: 16px;
                    font-style: italic;
                    color: #666;
                    padding: 10px;
                }
            """)
            self.start_button.setEnabled(False)
            self.start_button.setText("Start game")
        
        self.logger.info(f"Lobby updated: {room_id}, {player_count}/{self.max_players} players")
    
    def show_player_joined(self, player_name: str):
        """
        Show notification that a player joined.
        
        Args:
            player_name: Name of player who joined
        """
        self.status_label.setText(f"{player_name} joined the room!")
        self.status_label.setStyleSheet("""
            QLabel {
                font-size: 16px;
                font-weight: bold;
                color: #2196F3;
                padding: 10px;
            }
        """)
        
        self.logger.info(f"Player joined: {player_name}")
    
    def show_player_left(self, player_name: str):
        """
        Show notification that a player left.
        
        Args:
            player_name: Name of player who left
        """
        self.status_label.setText(f"{player_name} left the room.")
        self.status_label.setStyleSheet("""
            QLabel {
                font-size: 16px;
                font-weight: bold;
                color: #f44336;
                padding: 10px;
            }
        """)
        
        self.logger.info(f"Player left: {player_name}")
    
    def _on_start_clicked(self):
        """Handle start button click"""
        self.logger.info("Start game requested from lobby")
        
        # Disable button to prevent multiple clicks
        self.start_button.setEnabled(False)
        self.start_button.setText("Starting...")
        
        # Emit signal
        self.start_game_requested.emit()
    
    def reset(self):
        """Reset lobby to initial state"""
        self.room_id = ""
        self.player_count = 0
        self.players = []
        self.room_full = False
        
        self.room_id_label.setText("Room: -")
        self.player_count_label.setText("Players: 0/2")
        self.players_list.clear()
        self.status_label.setText("Waiting for players...")
        self.start_button.setEnabled(False)
        self.start_button.setText("Start Game")
        
        self.logger.info("Lobby reset")
