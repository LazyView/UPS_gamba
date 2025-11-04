"""
Game widget - main game screen with all game components.

This is the complete game interface that shows:
- Opponent info panel
- Game board (deck, top card)
- Your hand with cards
- Action buttons
- Game log
"""

from PyQt5.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QLabel, 
    QPushButton, QFrame, QScrollArea, QMessageBox
)
from PyQt5.QtCore import Qt, pyqtSignal
from PyQt5.QtGui import QFont

from game import Card, GameState, GameRules
from .widgets import CardWidget, EmptyCardSlot, PlayerInfoWidget, GameLogWidget


class GameWidget(QWidget):
    """
    Complete game interface widget.
    
    Responsibilities:
    - Display game state visually
    - Handle user input (card selection, button clicks)
    - Emit signals for game actions
    - Update UI based on game state
    
    Does NOT:
    - Handle network operations
    - Validate moves (GameRules does this)
    - Manage game state (GameState does this)
    
    Signals:
        play_cards_requested(cards_str): User wants to play cards
        pickup_pile_requested: User wants to pick up pile
        play_reserve_requested: User wants to play from reserve
    """
    
    # Signals
    play_cards_requested = pyqtSignal(str)  # Comma-separated card codes
    pickup_pile_requested = pyqtSignal()
    play_reserve_requested = pyqtSignal()
    
    def __init__(self, game_state: GameState, parent=None):
        """
        Initialize game widget.
        
        Args:
            game_state: GameState instance
            parent: Parent widget
        """
        super().__init__(parent)
        
        self.game_state = game_state
        self.selected_cards = []  # List of CardWidget objects
        
        self._setup_ui()
        
    def _setup_ui(self):
        """Setup user interface"""
        main_layout = QVBoxLayout()
        main_layout.setContentsMargins(10, 10, 10, 10)
        main_layout.setSpacing(10)
        
        # Top section: Opponent info + Game log
        top_layout = QHBoxLayout()
        
        self.opponent_info = PlayerInfoWidget()
        top_layout.addWidget(self.opponent_info)
        
        top_layout.addStretch()
        
        self.game_log = GameLogWidget()
        top_layout.addWidget(self.game_log)
        
        main_layout.addLayout(top_layout)
        
        # Separator
        separator1 = QFrame()
        separator1.setFrameShape(QFrame.HLine)
        separator1.setFrameShadow(QFrame.Sunken)
        main_layout.addWidget(separator1)
        
        # Center section: Game board
        board_layout = QVBoxLayout()
        board_layout.setSpacing(10)
        
        # Game board title
        board_title = QLabel("Game Board")
        board_title.setAlignment(Qt.AlignCenter)
        board_title.setStyleSheet("font-size: 14px; font-weight: bold; color: #2c3e50;")
        board_layout.addWidget(board_title)
        
        # Deck and top card
        cards_layout = QHBoxLayout()
        cards_layout.addStretch()
        
        # Deck (always empty in this game, but show placeholder)
        deck_container = QVBoxLayout()
        deck_label = QLabel("Deck")
        deck_label.setAlignment(Qt.AlignCenter)
        deck_label.setStyleSheet("font-size: 11px; color: #7f8c8d;")
        deck_container.addWidget(deck_label)
        
        # Deck size counter
        self.deck_count_label = QLabel("0") # Default to "0"
        self.deck_count_label.setAlignment(Qt.AlignCenter)
        self.deck_count_label.setStyleSheet("""
            QLabel {
                font-size: 18px;
                font-weight: bold;
                color: #7f8c8d; /* Lighter color for placeholder */
                background-color: #ecf0f1; /* Light gray background */
                border: 2px dashed #bdc3c7; /* Dashed border */
                border-radius: 8px;
            }
        """)
        self.deck_count_label.setMinimumSize(80, 110)
        
        
        deck_container.addWidget(self.deck_count_label)
        cards_layout.addLayout(deck_container)
        
        cards_layout.addSpacing(40)
        
        # Top card (discard pile) - STORE THE LAYOUT
        self.top_card_container = QVBoxLayout()
        top_card_label = QLabel("Top Card")
        top_card_label.setAlignment(Qt.AlignCenter)
        top_card_label.setStyleSheet("font-size: 11px; color: #7f8c8d;")
        self.top_card_container.addWidget(top_card_label)
        
        self.top_card_widget = EmptyCardSlot("None")
        self.top_card_container.addWidget(self.top_card_widget)
        cards_layout.addLayout(self.top_card_container)
        
        cards_layout.addStretch()
        board_layout.addLayout(cards_layout)
        
        # Special status (must play low indicator)
        self.special_status_label = QLabel("")
        self.special_status_label.setAlignment(Qt.AlignCenter)
        self.special_status_label.setStyleSheet("""
            font-size: 13px;
            font-weight: bold;
            color: #e67e22;
            background-color: #ffeaa7;
            padding: 8px;
            border-radius: 5px;
        """)
        self.special_status_label.setVisible(False)
        board_layout.addWidget(self.special_status_label)
        
        main_layout.addLayout(board_layout)
        
        # Separator
        separator2 = QFrame()
        separator2.setFrameShape(QFrame.HLine)
        separator2.setFrameShadow(QFrame.Sunken)
        main_layout.addWidget(separator2)
        
        # Turn indicator
        self.turn_indicator = QLabel("Waiting for game to start...")
        self.turn_indicator.setAlignment(Qt.AlignCenter)
        self.turn_indicator.setStyleSheet("""
            font-size: 16px;
            font-weight: bold;
            color: #95a5a6;
            padding: 10px;
            border-radius: 5px;
        """)
        main_layout.addWidget(self.turn_indicator)
        
        # Bottom section: Your hand + buttons
        hand_section = QVBoxLayout()
        hand_section.setSpacing(10)
        
        # Hand title
        hand_title_layout = QHBoxLayout()
        hand_title = QLabel("Your Hand")
        hand_title.setStyleSheet("font-size: 12px; font-weight: bold; color: #2c3e50;")
        hand_title_layout.addWidget(hand_title)
        
        # Reserve indicator
        self.reserve_label = QLabel("Reserve: 0")
        self.reserve_label.setStyleSheet("font-size: 11px; color: #7f8c8d; font-style: italic;")
        hand_title_layout.addWidget(self.reserve_label)
        
        hand_title_layout.addStretch()
        hand_section.addLayout(hand_title_layout)
        
        # Cards container (scrollable)
        hand_container = QHBoxLayout()
        
        self.hand_layout = QHBoxLayout()
        self.hand_layout.setSpacing(5)
        self.hand_layout.addStretch()
        
        hand_scroll = QScrollArea()
        hand_scroll.setWidgetResizable(True)
        hand_scroll.setHorizontalScrollBarPolicy(Qt.ScrollBarAsNeeded)
        hand_scroll.setVerticalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        hand_scroll.setMaximumHeight(140)
        hand_scroll.setStyleSheet("QScrollArea { border: none; }")
        
        hand_widget = QWidget()
        hand_widget.setLayout(self.hand_layout)
        hand_scroll.setWidget(hand_widget)
        
        hand_container.addWidget(hand_scroll)
        hand_section.addLayout(hand_container)
        
        # Action buttons
        button_layout = QHBoxLayout()
        button_layout.addStretch()
        
        self.play_button = QPushButton("Play Cards")
        self.play_button.setEnabled(False)
        self.play_button.clicked.connect(self._on_play_clicked)
        self.play_button.setMinimumWidth(120)
        self.play_button.setMinimumHeight(35)
        self.play_button.setStyleSheet("""
            QPushButton {
                background-color: #27ae60;
                color: white;
                font-size: 14px;
                font-weight: bold;
                border-radius: 5px;
                padding: 8px;
            }
            QPushButton:hover {
                background-color: #229954;
            }
            QPushButton:disabled {
                background-color: #bdc3c7;
                color: #7f8c8d;
            }
        """)
        button_layout.addWidget(self.play_button)
        
        self.pickup_button = QPushButton("Pick Up Pile")
        self.pickup_button.setEnabled(False)
        self.pickup_button.clicked.connect(self._on_pickup_clicked)
        self.pickup_button.setMinimumWidth(120)
        self.pickup_button.setMinimumHeight(35)
        self.pickup_button.setStyleSheet("""
            QPushButton {
                background-color: #e67e22;
                color: white;
                font-size: 14px;
                font-weight: bold;
                border-radius: 5px;
                padding: 8px;
            }
            QPushButton:hover {
                background-color: #d35400;
            }
            QPushButton:disabled {
                background-color: #bdc3c7;
                color: #7f8c8d;
            }
        """)
        button_layout.addWidget(self.pickup_button)
        
        self.reserve_button = QPushButton("Play Reserve")
        self.reserve_button.setEnabled(False)
        self.reserve_button.clicked.connect(self._on_reserve_clicked)
        self.reserve_button.setMinimumWidth(120)
        self.reserve_button.setMinimumHeight(35)
        self.reserve_button.setStyleSheet("""
            QPushButton {
                background-color: #9b59b6;
                color: white;
                font-size: 14px;
                font-weight: bold;
                border-radius: 5px;
                padding: 8px;
            }
            QPushButton:hover {
                background-color: #8e44ad;
            }
            QPushButton:disabled {
                background-color: #bdc3c7;
                color: #7f8c8d;
            }
        """)
        button_layout.addWidget(self.reserve_button)
        
        button_layout.addStretch()
        hand_section.addLayout(button_layout)
        
        main_layout.addLayout(hand_section)
        
        self.setLayout(main_layout)
    
    def update_game_state(self):
        """Update UI based on current game state"""
        # Update opponent info
        if self.game_state.opponent:
            self.opponent_info.update_info(
                self.game_state.opponent.name,
                self.game_state.opponent.hand_size,
                self.game_state.opponent.reserves_count
            )
            self.opponent_info.set_current_turn(
                not self.game_state.your_turn()
            )
        
        # Update deck size counter
        if self.game_state.deck_size:
            self.deck_count_label.setText(str(self.game_state.deck_size))
        # Update top card
        if self.game_state.top_card:
            # Remove old widget from layout
            if self.top_card_widget:
                self.top_card_container.removeWidget(self.top_card_widget)
                self.top_card_widget.deleteLater()
            
            # Create new card widget
            self.top_card_widget = CardWidget(self.game_state.top_card)
            self.top_card_widget.set_clickable(False)
            
            # Add to layout
            self.top_card_container.addWidget(self.top_card_widget)
        
        # Update special status
        if self.game_state.must_play_low:
            self.special_status_label.setText("⚠️ Must Play ≤7")
            self.special_status_label.setVisible(True)
        else:
            self.special_status_label.setVisible(False)
        
        # Update hand
        self._update_hand()
        
        # Update reserve label
        self.reserve_label.setText(f"Reserve: {self.game_state.player.reserves_count}")
        
        # Update turn indicator and buttons
        if self.game_state.your_turn():
            self.turn_indicator.setText("🎯 YOUR TURN")
            self.turn_indicator.setStyleSheet("""
                font-size: 16px;
                font-weight: bold;
                color: #27ae60;
                background-color: #d5f4e6;
                padding: 10px;
                border-radius: 5px;
            """)
            self._enable_actions()
        else:
            self.turn_indicator.setText("⏳ Opponent's Turn")
            self.turn_indicator.setStyleSheet("""
                font-size: 16px;
                font-weight: bold;
                color: #e67e22;
                background-color: #ffeaa7;
                padding: 10px;
                border-radius: 5px;
            """)
            self._disable_actions()
    
    def _update_hand(self):
        """Update hand display"""
        # Clear existing cards
        while self.hand_layout.count() > 1:  # Keep the stretch
            item = self.hand_layout.takeAt(0)
            if item.widget():
                item.widget().deleteLater()
        
        # Clear selection
        self.selected_cards = []
        
        # Add cards
        for card in self.game_state.player.hand:
            card_widget = CardWidget(card)
            card_widget.clicked.connect(self._on_card_clicked)
            self.hand_layout.insertWidget(self.hand_layout.count() - 1, card_widget)
    
    def _enable_actions(self):
        """Enable action buttons based on game state"""
        # Always allow pickup if it's your turn
        self.pickup_button.setEnabled(True)
        
        # Enable play button if cards are selected and valid
        self.play_button.setEnabled(len(self.selected_cards) > 0)
        
        # Enable reserve button if hand is empty and reserves exist
        if len(self.game_state.player.hand) == 0 and self.game_state.player.reserves_count > 0:
            self.reserve_button.setEnabled(True)
        else:
            self.reserve_button.setEnabled(False)
    
    def _disable_actions(self):
        """Disable all action buttons"""
        self.play_button.setEnabled(False)
        self.pickup_button.setEnabled(False)
        self.reserve_button.setEnabled(False)
    
    def _on_card_clicked(self, card: Card):
        """Handle card click"""
        # Find the widget
        for i in range(self.hand_layout.count() - 1):  # Exclude stretch
            widget = self.hand_layout.itemAt(i).widget()
            if isinstance(widget, CardWidget) and widget.get_card() == card:
                # Toggle selection
                widget.toggle_selected()
                
                if widget.is_selected:
                    self.selected_cards.append(widget)
                else:
                    self.selected_cards.remove(widget)
                
                # Update play button
                self.play_button.setEnabled(len(self.selected_cards) > 0)
                break
    
    def _on_play_clicked(self):
        """Handle play button click"""
        if not self.selected_cards:
            return
        
        # Get selected card objects
        cards = [widget.get_card() for widget in self.selected_cards]
        
        # Validate play
        is_valid, error = GameRules.validate_card_play(
            self.game_state.player,
            cards,
            self.game_state.top_card,
            self.game_state.must_play_low
        )
        
        if not is_valid:
            QMessageBox.warning(self, "Invalid Play", error)
            return
        
        # Build card string
        cards_str = ','.join(card.code for card in cards)
        
        # Emit signal
        self.play_cards_requested.emit(cards_str)
        
        # Log
        self.game_log.add_player_action("You", f"played {cards_str}")
    
    def _on_pickup_clicked(self):
        """Handle pickup button click"""
        # Emit signal
        self.pickup_pile_requested.emit()
        
        # Log
        self.game_log.add_player_action("You", "picked up the pile")
    
    def _on_reserve_clicked(self):
        """Handle reserve button click"""
        # Emit signal
        self.play_reserve_requested.emit()
        
        # Log
        self.game_log.add_player_action("You", "played from reserve")
    
    def add_log_message(self, message: str, msg_type: str = "normal"):
        """
        Add message to game log.
        
        Args:
            message: Message text
            msg_type: Message type ('normal', 'system', 'error', 'success')
        """
        self.game_log.add_message(message, msg_type)
    
    def reset(self):
        """Reset game widget to initial state"""
        self.selected_cards = []
        self.game_log.clear()
        self.opponent_info.reset()
        self._update_hand()
        self.turn_indicator.setText("Waiting for game to start...")
        self.turn_indicator.setStyleSheet("""
            font-size: 16px;
            font-weight: bold;
            color: #95a5a6;
            padding: 10px;
            border-radius: 5px;
        """)
        self._disable_actions()
