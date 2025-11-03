"""
Main application window - orchestrates the entire application.

This is the top-level coordinator that:
1. Creates and manages all major components (ConnectionManager, GameState, Widgets)
2. Connects signals between components
3. Switches between application screens (connection, lobby, game)
4. Provides global UI elements (menu bar, status bar)

Separation of Concerns:
- MainWindow: UI orchestration and component coordination
- ConnectionManager: All network operations
- GameState: All game logic
- Widgets: UI display and user input
"""

from PyQt5.QtWidgets import (
    QMainWindow, QWidget, QVBoxLayout, QStackedWidget,
    QMessageBox, QMenuBar, QMenu, QAction, QStatusBar
)
from PyQt5.QtCore import Qt, QTimer

from network import ConnectionManager
from game import GameState
from message import ServerMessageType
from utils import get_logger, log_game_event, constants

from .connection_dialog import ConnectionDialog
from .lobby_widget import LobbyWidget
from .game_widget import GameWidget


class MainWindow(QMainWindow):
    """
    Main application window.
    
    Coordinates:
    - ConnectionManager (network layer)
    - GameState (game logic)
    - UI widgets (connection, lobby, game)
    
    Responsibilities:
    - Show appropriate screen based on state
    - Connect signals between components
    - Handle global UI state (menus, status)
    - Error handling and user notifications
    
    Does NOT:
    - Handle network operations (ConnectionManager does this)
    - Handle game logic (GameState does this)
    - Handle layout details (widgets do this)
    """
    
    def __init__(self):
        """Initialize main window"""
        super().__init__()
        
        self.logger = get_logger()
        
        # Components
        self.connection_manager = ConnectionManager()
        self.game_state = GameState()
        
        # Widgets (will be created as needed)
        self.connection_dialog = None
        self.lobby_widget = None
        self.game_widget = None
        
        # UI state
        self.current_screen = None
        
        # Setup
        self._setup_window()
        self._setup_menu_bar()
        self._setup_status_bar()
        self._setup_central_widget()
        self._connect_signals()
        
        # Show connection dialog on startup
        QTimer.singleShot(100, self._show_connection_dialog)
        
        self.logger.info("MainWindow initialized")
    
    # ========================================================================
    # SETUP METHODS
    # ========================================================================
    
    def _setup_window(self):
        """Setup main window properties"""
        self.setWindowTitle("Gamba Card Game")
        self.setMinimumSize(800, 600)
        self.resize(1000, 700)
    
    def _setup_menu_bar(self):
        """Setup menu bar"""
        menubar = self.menuBar()
        
        # File menu
        file_menu = menubar.addMenu("&File")
        
        connect_action = QAction("&Connect", self)
        connect_action.setShortcut("Ctrl+N")
        connect_action.triggered.connect(self._show_connection_dialog)
        file_menu.addAction(connect_action)
        
        disconnect_action = QAction("&Disconnect", self)
        disconnect_action.setShortcut("Ctrl+D")
        disconnect_action.triggered.connect(self._disconnect)
        file_menu.addAction(disconnect_action)
        
        file_menu.addSeparator()
        
        exit_action = QAction("E&xit", self)
        exit_action.setShortcut("Ctrl+Q")
        exit_action.triggered.connect(self.close)
        file_menu.addAction(exit_action)
        
        # Help menu
        help_menu = menubar.addMenu("&Help")
        
        about_action = QAction("&About", self)
        about_action.triggered.connect(self._show_about)
        help_menu.addAction(about_action)
    
    def _setup_status_bar(self):
        """Setup status bar"""
        self.status_bar = QStatusBar()
        self.setStatusBar(self.status_bar)
        self._update_status("Not connected")
    
    def _setup_central_widget(self):
        """Setup central widget with stacked layout"""
        # Central widget
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        
        # Stacked widget for switching screens
        self.stacked_widget = QStackedWidget()
        
        layout = QVBoxLayout()
        layout.addWidget(self.stacked_widget)
        layout.setContentsMargins(0, 0, 0, 0)
        central_widget.setLayout(layout)
    
    def _connect_signals(self):
        """Connect signals from ConnectionManager"""
        # Connection state changes
        self.connection_manager.state_changed.connect(self._on_state_changed)
        
        # Messages from server
        self.connection_manager.message_received.connect(self._on_message_received)
        
        # Errors
        self.connection_manager.error_occurred.connect(self._on_error)
        
        # Reconnection
        self.connection_manager.reconnecting.connect(self._on_reconnecting)
        self.connection_manager.reconnected.connect(self._on_reconnected)
    
    # ========================================================================
    # SCREEN MANAGEMENT
    # ========================================================================
    
    def _show_connection_dialog(self):
        """Show connection dialog"""
        # Create dialog if needed
        if not self.connection_dialog:
            self.connection_dialog = ConnectionDialog(self)
            self.connection_dialog.connect_requested.connect(self._on_connect_requested)
        
        # Show dialog
        self.connection_dialog.show()
        self.connection_dialog.raise_()
        self.connection_dialog.activateWindow()
    
    def _show_lobby_screen(self):
        """Show lobby screen"""
        self._update_status("In lobby - waiting for players")
        self.logger.info("Showing lobby screen")
        
        # Create lobby widget if needed
        if not self.lobby_widget:
            self.lobby_widget = LobbyWidget(self)
            self.lobby_widget.start_game_requested.connect(self._on_start_game_requested)
            self.stacked_widget.addWidget(self.lobby_widget)
        
        # Switch to lobby
        self.stacked_widget.setCurrentWidget(self.lobby_widget)
        self.current_screen = "lobby"
    
    def _show_game_screen(self):
        """Show game screen"""
        self._update_status("In game")
        self.logger.info("Showing game screen")
        
        # Create game widget if needed
        if not self.game_widget:
            self.game_widget = GameWidget(self.game_state, self)
            
            # Connect game widget signals to connection manager
            self.game_widget.play_cards_requested.connect(self._on_play_cards_requested)
            self.game_widget.pickup_pile_requested.connect(self._on_pickup_pile_requested)
            self.game_widget.play_reserve_requested.connect(self._on_play_reserve_requested)
            
            self.stacked_widget.addWidget(self.game_widget)
        
        # Update game widget with current state
        self.game_widget.update_game_state()
        self.game_widget.add_log_message("Game started!", "system")
        
        # Switch to game screen
        self.stacked_widget.setCurrentWidget(self.game_widget)
        self.current_screen = "game"
    
    # ========================================================================
    # CONNECTION HANDLERS
    # ========================================================================
    
    def _on_connect_requested(self, host: str, port: int, name: str):
        """
        Handle connection request from dialog.
        
        Args:
            host: Server host
            port: Server port
            name: Player name
        """
        self.logger.info(f"Connection requested: {host}:{port} as '{name}'")
        
        # Initialize game state with player name
        self.game_state.initialize_player(name)
        
        # Attempt connection
        success = self.connection_manager.connect(host, port, name)
        
        if not success:
            # Connection failed immediately
            if self.connection_dialog:
                self.connection_dialog.show_error("Failed to connect to server")
    
    def _disconnect(self):
        """Disconnect from server"""
        if self.connection_manager.is_connected():
            reply = QMessageBox.question(
                self,
                "Disconnect",
                "Are you sure you want to disconnect?",
                QMessageBox.Yes | QMessageBox.No,
                QMessageBox.No
            )
            
            if reply == QMessageBox.Yes:
                self.connection_manager.disconnect()
                self._update_status("Disconnected")

    def _on_start_game_requested(self):
        """Handle start game request from lobby"""
        self.logger.info("Start game requested")
        self.connection_manager.send_start_game()
    
    # ========================================================================
    # GAME ACTION HANDLERS
    # ========================================================================
    
    def _on_play_cards_requested(self, cards_str: str):
        """
        Handle play cards request from game widget.
        
        Args:
            cards_str: Comma-separated card codes
        """
        self.logger.info(f"Play cards requested: {cards_str}")
        self.connection_manager.send_play_cards(cards_str)
    
    def _on_pickup_pile_requested(self):
        """Handle pickup pile request from game widget"""
        self.logger.info("Pickup pile requested")
        self.connection_manager.send_pickup_pile()
    
    def _on_play_reserve_requested(self):
        """Handle play reserve request from game widget"""
        self.logger.info("Play reserve requested")
        self.connection_manager.send_play_cards("RESERVE")
    
    # ========================================================================
    # CONNECTION MANAGER SIGNAL HANDLERS
    # ========================================================================
    
    def _on_state_changed(self, old_state: str, new_state: str):
        """
        Handle connection state change.
        
        Args:
            old_state: Previous state
            new_state: New state
        """
        self.logger.info(f"State changed: {old_state} → {new_state}")
        
        # Update UI based on state
        if new_state == constants.STATE_CONNECTED:
            # Successfully connected
            if self.connection_dialog:
                self.connection_dialog.show_success()
            self._update_status("Connected to server")

            # Auto-join room after connection
            QTimer.singleShot(100, self.connection_manager.send_join_room)
            
        elif new_state == constants.STATE_IN_ROOM:
            # Joined room - show lobby
            self._show_lobby_screen()
            
        elif new_state == constants.STATE_IN_GAME:
            # Game started - show game screen
            self._show_game_screen()
            
        elif new_state == constants.STATE_DISCONNECTED:
            # Disconnected
            self._update_status("Not connected")
            
            # If we were in a screen, show connection dialog
            if self.current_screen:
                self._show_connection_dialog()
                self.current_screen = None
    
    def _on_message_received(self, message: dict):
        """
        Handle message from server.
        
        Args:
            message: Parsed message dictionary
        """
        msg_type = message.get('type')
        
        # Handle specific messages
        if msg_type == ServerMessageType.ERROR:
            error_msg = message.get('data', {}).get('error', 'Unknown error')
            self._show_error(f"Server error: {error_msg}")
            if self.game_widget:
                self.game_widget.add_log_message(f"Error: {error_msg}", "error")
            
        elif msg_type == ServerMessageType.GAME_STATE:
            # Update game state
            self.game_state.update_from_game_state_message(message.get('data', {}))
            
            # Update game widget if visible
            if self.game_widget and self.current_screen == "game":
                self.game_widget.update_game_state()
            
        elif msg_type == ServerMessageType.TURN_RESULT:
            # Log turn result
            result = message.get('data', {}).get('result', '')
            status = message.get('data', {}).get('status', '')
            
            if self.game_widget:
                if status == 'success':
                    self.game_widget.add_log_message(f"Move successful: {result}", "success")
                else:
                    self.game_widget.add_log_message(f"Move failed: {result}", "error")
            
        elif msg_type == ServerMessageType.GAME_OVER:
            winner = message.get('data', {}).get('winner')
            reason = message.get('data', {}).get('reason', '')
            
            if self.game_widget:
                self.game_widget.add_log_message(f"Game Over! Winner: {winner}", "highlight")
            
            self._show_game_over(winner)
        
        elif msg_type == ServerMessageType.ROOM_JOINED:
            # Update lobby with room info
            if self.lobby_widget:
                data = message.get('data', {})
                room_id = message.get('room', 'UNKNOWN')
                player_count = int(data.get('player_count', 0))
                players = data.get('players', '').split(',') if data.get('players') else []
                room_full = data.get('room_full', 'false') == 'true'
                
                self.lobby_widget.update_room_info(room_id, player_count, players, room_full)
                
                # Check for player joined broadcast
                if data.get('broadcast_type') == 'room_notification':
                    joined_player = data.get('joined_player')
                    if joined_player:
                        self.lobby_widget.show_player_joined(joined_player)
        
        elif msg_type == ServerMessageType.PLAYER_DISCONNECTED:
            disconnected_player = message.get('data', {}).get('disconnected_player')
            if self.game_widget:
                self.game_widget.add_log_message(f"{disconnected_player} disconnected", "error")
                if self.game_state.opponent and self.game_state.opponent.name == disconnected_player:
                    self.game_widget.opponent_info.set_connected(False)
        
        elif msg_type == ServerMessageType.PLAYER_RECONNECTED:
            reconnected_player = message.get('data', {}).get('reconnected_player')
            if self.game_widget:
                self.game_widget.add_log_message(f"{reconnected_player} reconnected", "success")
                if self.game_state.opponent and self.game_state.opponent.name == reconnected_player:
                    self.game_widget.opponent_info.set_connected(True)
    
    def _on_error(self, error_msg: str):
        """
        Handle error from connection manager.
        
        Args:
            error_msg: Error message
        """
        self.logger.error(f"Connection error: {error_msg}")
        self._show_error(error_msg)
        
        # Show error in connection dialog if it's open
        if self.connection_dialog and self.connection_dialog.isVisible():
            self.connection_dialog.show_error(error_msg)
    
    def _on_reconnecting(self):
        """Handle reconnection attempt"""
        self._update_status("Reconnecting...")
        if self.game_widget:
            self.game_widget.add_log_message("Reconnecting...", "system")
    
    def _on_reconnected(self):
        """Handle successful reconnection"""
        self._update_status("Reconnected")
        if self.game_widget:
            self.game_widget.add_log_message("Reconnected!", "success")
        
        QMessageBox.information(
            self,
            "Reconnected",
            "Successfully reconnected to server!",
            QMessageBox.Ok
        )
    
    # ========================================================================
    # UI HELPERS
    # ========================================================================
    
    def _update_status(self, message: str):
        """
        Update status bar message.
        
        Args:
            message: Status message
        """
        self.status_bar.showMessage(message)
    
    def _show_error(self, message: str):
        """
        Show error dialog.
        
        Args:
            message: Error message
        """
        QMessageBox.critical(
            self,
            "Error",
            message,
            QMessageBox.Ok
        )
    
    def _show_game_over(self, winner: str):
        """
        Show game over dialog.
        
        Args:
            winner: Name of winning player
        """
        player_name = self.game_state.player_name
        
        if winner == player_name:
            title = "You Won!"
            message = "Congratulations! You won the game!"
        else:
            title = "Game Over"
            message = f"{winner} won the game."
        
        QMessageBox.information(
            self,
            title,
            message,
            QMessageBox.Ok
        )
        
        log_game_event("GAME_OVER", f"Winner: {winner}")
    
    def _show_about(self):
        """Show about dialog"""
        QMessageBox.about(
            self,
            "About Gamba",
            "<h2>Gamba Card Game</h2>"
            "<p>A multiplayer card game client.</p>"
            "<p><b>Version:</b> 1.0</p>"
            "<p><b>Author:</b> Your Name</p>"
            "<p>Built with PyQt5 and Python.</p>"
        )
    
    # ========================================================================
    # WINDOW EVENTS
    # ========================================================================
    
    def closeEvent(self, event):
        """Handle window close event"""
        # Ask for confirmation if connected
        if self.connection_manager.is_connected():
            reply = QMessageBox.question(
                self,
                "Exit",
                "Are you sure you want to exit?\nYou will be disconnected from the game.",
                QMessageBox.Yes | QMessageBox.No,
                QMessageBox.No
            )
            
            if reply == QMessageBox.No:
                event.ignore()
                return
        
        # Disconnect
        self.connection_manager.disconnect()
        
        # Accept close
        event.accept()
        self.logger.info("MainWindow closed")
