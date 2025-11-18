"""
Connection manager - orchestrates network client and heartbeat.
Handles state machine and reconnection logic.
"""

from typing import Optional, Callable
from PyQt5.QtCore import QObject, pyqtSignal, QTimer
from datetime import datetime, timedelta

from .client import NetworkClient
from .heartbeat_manager import HeartbeatManager
from message import MessageProtocol, ClientMessageType, ServerMessageType
from game import GameState
from utils import (
    get_logger,
    log_state_change,
    log_connection_event,
    log_error,
    constants
)


class ConnectionManager(QObject):
    """
    High-level connection manager.
    
    Responsibilities:
    - Manage connection state machine
    - Coordinate NetworkClient and HeartbeatManager
    - Handle reconnection (automatic short-term, manual long-term)
    - Provide simple API for UI layer
    
    State Machine:
    DISCONNECTED → CONNECTING → CONNECTED → IN_LOBBY → IN_ROOM → IN_GAME
                      ↓             ↓           ↓         ↓          ↓
                   DISCONNECTED ←──────────────────────────────────┘
                      ↓
                   RECONNECTING → back to appropriate state
    
    Signals:
    - state_changed(old_state, new_state): State transition
    - message_received(message_dict): Incoming message
    - error_occurred(error_msg): Error
    - reconnecting: Auto-reconnection started
    - reconnected: Reconnection successful
    """
    
    # Signals
    state_changed = pyqtSignal(str, str)  # (old_state, new_state)
    message_received = pyqtSignal(dict)  # Incoming message
    error_occurred = pyqtSignal(str)  # Error message
    reconnecting = pyqtSignal()  # Auto-reconnection started
    reconnected = pyqtSignal()  # Reconnection successful
    reconnect_status = pyqtSignal(int)  # Reconnection progress (seconds remaining)
    
    def __init__(self):
        """Initialize connection manager"""
        super().__init__()
        
        self.logger = get_logger()
        
        # Components
        self.network_client: Optional[NetworkClient] = None
        self.heartbeat: Optional[HeartbeatManager] = None
        
        # State
        self.state = constants.STATE_DISCONNECTED
        self.player_name: str = ""
        self.disconnect_time: Optional[datetime] = None
        self.reconnect_attempts = 0
        self.intentional_disconnect = False
        
        # Reconnection timer
        self.reconnect_timer = QTimer()
        self.reconnect_timer.timeout.connect(self._attempt_reconnect)
        
        self.logger.info("ConnectionManager initialized")
    
    # ========================================================================
    # PUBLIC API - Connection Control
    # ========================================================================
    
    def connect(self, host: str, port: int, player_name: str) -> bool:
        """
        Initiate connection to server.
        
        Args:
            host: Server host
            port: Server port
            player_name: Player name for CONNECT message
            
        Returns:
            True if connection initiated
        """
        if self.state != constants.STATE_DISCONNECTED:
            self.logger.warning(f"Cannot connect - current state: {self.state}")
            return False
        
        self.player_name = player_name
        
        # Store host/port for reconnection
        self._last_host = host
        self._last_port = port
        
        # Change state
        self._change_state(constants.STATE_CONNECTING)
        
        # Create network client
        self.network_client = NetworkClient(host, port)
        self._connect_network_signals()
        
        # Start network thread
        self.network_client.start()
        
        # Initiate connection
        success = self.network_client.connect_to_server()
        
        if not success:
            self._change_state(constants.STATE_DISCONNECTED)
            self.network_client.stop()
            self.network_client = None
        
        return success
    
    def disconnect(self):
        """Disconnect from server"""
        self.logger.info("Disconnect requested")

        # Mark as intentional
        self.intentional_disconnect = True
        
        # Stop reconnection if active
        self.reconnect_timer.stop()
        
        # Stop heartbeat
        if self.heartbeat:
            self.heartbeat.stop()
            self.heartbeat = None
        
        # Stop network client
        if self.network_client:
            self.network_client.stop()
            self.network_client = None
        
        # Change state
        self._change_state(constants.STATE_DISCONNECTED)
    
    def reconnect_manually(self) -> bool:
        """
        Manually trigger reconnection (for temporary disconnects).
        Requires active session with saved connection info.

        Security: Only works within the same client session to prevent
        session hijacking. After client restart, use Connect instead.

        Returns:
            True if reconnection initiated
        """
        if self.state not in [constants.STATE_DISCONNECTED, constants.STATE_RECONNECTING]:
            self.logger.warning(f"Cannot reconnect - current state: {self.state}")
            return False

        if not hasattr(self, '_last_host') or not hasattr(self, '_last_port'):
            self.logger.error("Cannot reconnect - no previous connection info")
            return False

        self.logger.info("Manual reconnection requested")

        # Stop any ongoing auto-reconnect
        self.reconnect_timer.stop()

        # Reset reconnect state
        self.disconnect_time = datetime.now()
        self.reconnect_attempts = 0

        # Start reconnection
        self._start_auto_reconnect()

        return True
    
    def send_message(self, message: str):
        """
        Send message to server.
        
        Args:
            message: Protocol message string
        """
        if not self.network_client or not self.network_client.is_connected():
            self.logger.warning("Cannot send message - not connected")
            return
        
        self.network_client.send_message(message)
    
    # ========================================================================
    # CONVENIENCE METHODS - Protocol Messages
    # ========================================================================
    
    def send_connect(self, player_name: str):
        """Send CONNECT message"""
        msg = MessageProtocol.build(ClientMessageType.CONNECT, name=player_name)
        self.send_message(msg)
    
    def send_join_room(self):
        """Send JOIN_ROOM message"""
        msg = MessageProtocol.build(ClientMessageType.JOIN_ROOM)
        self.send_message(msg)
    
    def send_start_game(self):
        """Send START_GAME message"""
        msg = MessageProtocol.build(ClientMessageType.START_GAME)
        self.send_message(msg)
    
    def send_play_cards(self, cards: str):
        """
        Send PLAY_CARDS message.
        
        Args:
            cards: Comma-separated card codes or "RESERVE"
        """
        msg = MessageProtocol.build(ClientMessageType.PLAY_CARDS, cards=cards)
        self.send_message(msg)
    
    def send_pickup_pile(self):
        """Send PICKUP_PILE message"""
        msg = MessageProtocol.build(ClientMessageType.PICKUP_PILE)
        self.send_message(msg)
    
    def send_reconnect(self, player_name: str):
        """Send RECONNECT message"""
        msg = MessageProtocol.build(ClientMessageType.RECONNECT, name=player_name)
        self.send_message(msg)
    
    # ========================================================================
    # STATE MANAGEMENT
    # ========================================================================
    
    def get_state(self) -> str:
        """Get current connection state"""
        return self.state
    
    def is_connected(self) -> bool:
        """Check if connected to server"""
        return self.network_client and self.network_client.is_connected()
    
    def is_in_game(self) -> bool:
        """Check if in active game"""
        return self.state == constants.STATE_IN_GAME
    
    def _change_state(self, new_state: str):
        """
        Change connection state.
        
        Args:
            new_state: New state constant
        """
        if new_state == self.state:
            return
        
        old_state = self.state
        self.state = new_state
        
        log_state_change(old_state, new_state)
        self.state_changed.emit(old_state, new_state)
    
    # ========================================================================
    # RECONNECTION LOGIC
    # ========================================================================
    
    def _handle_disconnection(self):
        """Handle unexpected disconnection"""
        # Only set disconnect_time on initial disconnect, not during reconnection attempts
        if self.state != constants.STATE_RECONNECTING:
            self.disconnect_time = datetime.now()

        # Stop heartbeat
        if self.heartbeat:
            self.heartbeat.stop()

        # Start auto-reconnection (or continue existing reconnection)
        if self.state != constants.STATE_RECONNECTING:
            self._start_auto_reconnect()
    
    def _start_auto_reconnect(self):
        """Start automatic reconnection attempts"""
        self.logger.info("Starting auto-reconnection")
        log_connection_event("AUTO_RECONNECT_START")
        
        self.reconnect_attempts = 0
        self._change_state(constants.STATE_RECONNECTING)
        self.reconnecting.emit()
        
        # Start reconnection timer
        self.reconnect_timer.start(constants.RECONNECT_RETRY_INTERVAL * 1000)
    
    def _attempt_reconnect(self):
        """Attempt to reconnect"""
        self.reconnect_attempts += 1

        # Check if we're within the reconnect window
        if self.disconnect_time:
            elapsed = (datetime.now() - self.disconnect_time).total_seconds()
            time_remaining = int(constants.SHORT_TERM_DISCONNECT_THRESHOLD - elapsed)

            if elapsed > constants.SHORT_TERM_DISCONNECT_THRESHOLD:
                # Short-term window expired - stop auto-reconnect
                # Player can still manually reconnect (60-120s window)
                self.logger.info(f"Disconnect exceeded {constants.SHORT_TERM_DISCONNECT_THRESHOLD}s - stopping auto-reconnect")
                self.reconnect_timer.stop()
                self._change_state(constants.STATE_DISCONNECTED)
                self.error_occurred.emit(
                    "Auto-reconnect stopped.\n\n"
                    "You can still manually reconnect within the next 60 seconds.\n"
                    "Please use File → Reconnect to continue your session.\n\n"
                    "After 120 seconds total, your session will expire and you'll need to use File → Connect."
                )
                return

            # Emit status update with time remaining
            self.logger.info(f"Reconnect attempt #{self.reconnect_attempts} ({time_remaining}s remaining)")
            self.reconnect_status.emit(time_remaining)
        
        # Attempt reconnection
        # Always create a fresh NetworkClient for reconnection
        # (old client's thread may have stopped after disconnect)
        if self.network_client:
            # Stop old client if it exists
            self.network_client.stop()
            self.network_client = None

        # Create new client with same host/port
        host = getattr(self, '_last_host', constants.DEFAULT_HOST)
        port = getattr(self, '_last_port', constants.DEFAULT_PORT)
        self.network_client = NetworkClient(host, port)
        self._connect_network_signals()
        self.network_client.start()

        # Try to reconnect
        if self.network_client.connect_to_server():
            # Connection successful - stop timer and wait for server response
            self.reconnect_timer.stop()
            # Send RECONNECT message
            self.send_reconnect(self.player_name)
    
    def _on_reconnect_success(self):
        """Handle successful reconnection"""
        self.logger.info("Reconnection successful")
        log_connection_event("RECONNECTED")
        
        self.reconnect_timer.stop()
        self.disconnect_time = None
        self.reconnect_attempts = 0
        
        # Restart heartbeat
        if not self.heartbeat:
            self.heartbeat = HeartbeatManager()
            self._connect_heartbeat_signals()
        self.heartbeat.start(self.network_client.send_message)
        
        self.reconnected.emit()
    
    # ========================================================================
    # SIGNAL HANDLERS
    # ========================================================================
    
    def _connect_network_signals(self):
        """Connect NetworkClient signals"""
        if not self.network_client:
            return
        
        self.network_client.connected.connect(self._on_network_connected)
        self.network_client.disconnected.connect(self._on_network_disconnected)
        self.network_client.message_received.connect(self._on_message_received)
        self.network_client.error_occurred.connect(self._on_network_error)
    
    def _connect_heartbeat_signals(self):
        """Connect HeartbeatManager signals"""
        if not self.heartbeat:
            return
        
        self.heartbeat.timeout_detected.connect(self._on_heartbeat_timeout)
    
    def _on_network_connected(self):
        """Handle network connected signal"""
        self.logger.info("Network connected")

        # Don't send CONNECT if we're reconnecting - RECONNECT will be sent instead
        if self.state == constants.STATE_RECONNECTING:
            self.logger.info("Reconnecting - RECONNECT message will be sent by _attempt_reconnect()")
            return

        # Send CONNECT message for normal connection
        self.send_connect(self.player_name)
    
    def _on_network_disconnected(self):
        """Handle network disconnected signal"""
        self.logger.info("Network disconnected")
        
        # Only handle if we didn't initiate disconnect
        if self.state != constants.STATE_DISCONNECTED and not self.intentional_disconnect:
            self._handle_disconnection()
    
        # Reset flag
        self.intentional_disconnect = False
    
    def _on_message_received(self, message_dict: dict):
        """
        Handle incoming message from server.

        Args:
            message_dict: Parsed message dictionary
        """
        msg_type = message_dict.get('type')

        # Handle protocol messages that affect connection state
        if msg_type == ServerMessageType.ERROR:
            # Handle ERROR during connection phase
            if self.state == constants.STATE_CONNECTING:
                self.logger.warning("Connection rejected by server")
                # Mark as intentional to prevent auto-reconnect
                self.intentional_disconnect = True
                # Disconnect and return to DISCONNECTED state
                if self.network_client:
                    self.network_client.stop()
                    self.network_client = None
                self._change_state(constants.STATE_DISCONNECTED)
        elif msg_type == ServerMessageType.CONNECTED:
            self._on_connected_message(message_dict)
        elif msg_type == ServerMessageType.PONG:
            self._on_pong_message()
        elif msg_type == ServerMessageType.ROOM_JOINED:
            self._change_state(constants.STATE_IN_ROOM)
        elif msg_type == ServerMessageType.ROOM_LEFT:
            # Player left room - back to connected state
            self.logger.info("Left room - changing to CONNECTED state")
            self._change_state(constants.STATE_CONNECTED)
        elif msg_type == ServerMessageType.GAME_STARTED:
            self._change_state(constants.STATE_IN_GAME)
        elif msg_type == ServerMessageType.GAME_STATE:
            # GAME_STATE indicates we're in an active game (e.g., after reconnection)
            if self.state != constants.STATE_IN_GAME:
                self.logger.info("Received GAME_STATE - changing to IN_GAME state")
                self._change_state(constants.STATE_IN_GAME)
        elif msg_type == ServerMessageType.TURN_UPDATE:
            # TURN_UPDATE is only sent during active gameplay
            # State should already be IN_GAME, but ensure it
            if self.state != constants.STATE_IN_GAME:
                self.logger.warning("Received TURN_UPDATE while not IN_GAME - fixing state")
                self._change_state(constants.STATE_IN_GAME)
        elif msg_type == ServerMessageType.GAME_OVER:
            # Don't change state here - let ROOM_LEFT handle it
            # This prevents auto-joining before server sends ROOM_LEFT
            self.logger.info("Game over - waiting for ROOM_LEFT")

        # Forward message to UI
        self.message_received.emit(message_dict)
    
    def _on_connected_message(self, message_dict: dict):
        """Handle CONNECTED message from server"""
        self.logger.debug(f"CONNECTED message received: {message_dict}")
        status = message_dict.get('data', {}).get('status')  # Parser expands 'st' to 'status'
        self.logger.debug(f"Status value: {repr(status)}")

        if status == 'success':  # Parser expands 'ok' to 'success'
            self.logger.info("Connection successful")
            self._change_state(constants.STATE_CONNECTED)
            
            # Start heartbeat
            if not self.heartbeat:
                self.heartbeat = HeartbeatManager()
                self._connect_heartbeat_signals()
            self.heartbeat.start(self.network_client.send_message)
            
            # Check if this was a reconnection
            if self.reconnect_attempts > 0:
                self._on_reconnect_success()
    
    def _on_pong_message(self):
        """Handle PONG message from server"""
        if self.heartbeat:
            self.heartbeat.on_pong_received()
    
    def _on_network_error(self, error_msg: str):
        """Handle network error"""
        self.logger.error(f"Network error: {error_msg}")
        self.error_occurred.emit(error_msg)
    
    def _on_heartbeat_timeout(self):
        """Handle heartbeat timeout"""
        self.logger.warning("Heartbeat timeout detected")
        log_connection_event("HEARTBEAT_TIMEOUT")
        
        # Treat as disconnection
        if self.network_client:
            self.network_client.disconnect_from_server()
    
    def __del__(self):
        """Cleanup on destruction"""
        self.disconnect()
