"""
Network client for Gamba - handles TCP communication in separate thread.

CRITICAL: This runs in a separate QThread to avoid blocking the GUI.
All communication with the main thread is via Qt signals (thread-safe).
"""

import socket
import time
from typing import Optional
from PyQt5.QtCore import QThread, pyqtSignal

from message import MessageBuffer, MessageProtocol
from utils import (
    get_logger,
    log_message_sent,
    log_message_received,
    log_connection_event,
    log_error,
    constants
)


class NetworkClient(QThread):
    """
    TCP network client running in separate thread.
    
    Responsibilities:
    - Establish/close TCP connection
    - Send messages to server
    - Receive messages from server (blocking recv in thread)
    - Parse incoming messages
    - Emit signals for all events
    
    Signals:
    - connected: Emitted when TCP connection established
    - disconnected: Emitted when connection lost
    - message_received: Emitted when valid message parsed
    - error_occurred: Emitted on any error
    - connection_timeout: Emitted when no data received for timeout period
    
    Usage:
        client = NetworkClient("127.0.0.1", 8080)
        client.message_received.connect(on_message)
        client.connected.connect(on_connected)
        client.start()  # Start thread
        client.connect_to_server()
    """
    
    # Signals (thread-safe communication)
    connected = pyqtSignal()  # TCP connection established
    disconnected = pyqtSignal()  # Connection lost
    message_received = pyqtSignal(dict)  # Parsed message (as dict)
    error_occurred = pyqtSignal(str)  # Error message
    connection_timeout = pyqtSignal()  # No data received within timeout
    
    def __init__(self, host: str = constants.DEFAULT_HOST, port: int = constants.DEFAULT_PORT):
        """
        Initialize network client.
        
        Args:
            host: Server host address
            port: Server port
        """
        super().__init__()
        
        self.host = host
        self.port = port
        self.socket: Optional[socket.socket] = None
        self.running = False
        self.connected_flag = False
        
        # Message buffer for handling partial TCP messages
        self.buffer = MessageBuffer()
        
        self.logger = get_logger()
        self.logger.info(f"NetworkClient initialized for {host}:{port}")
    
    def set_server(self, host: str, port: int):
        """
        Update server address (only when disconnected).
        
        Args:
            host: Server host
            port: Server port
        """
        if self.connected_flag:
            self.logger.warning("Cannot change server while connected")
            return
        
        self.host = host
        self.port = port
        self.logger.info(f"Server updated to {host}:{port}")
    
    def connect_to_server(self) -> bool:
        """
        Establish TCP connection to server.
        Should be called after thread is started.
        
        Returns:
            True if connection initiated (actual result via signals)
        """
        if self.connected_flag:
            self.logger.warning("Already connected")
            return False
        
        self.logger.info(f"Initiating connection to {self.host}:{self.port}")
        
        try:
            # Create socket
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.settimeout(constants.SOCKET_TIMEOUT)
            
            # Connect
            self.socket.connect((self.host, self.port))
            
            # Mark as connected
            self.connected_flag = True
            
            log_connection_event("CONNECTED", f"{self.host}:{self.port}")
            self.connected.emit()
            
            return True
            
        except ConnectionRefusedError:
            error_msg = constants.ERROR_CONNECTION_REFUSED
            log_error(error_msg)
            self.error_occurred.emit(error_msg)
            self._cleanup_socket()
            return False
            
        except socket.timeout:
            error_msg = "Connection timeout"
            log_error(error_msg)
            self.error_occurred.emit(error_msg)
            self._cleanup_socket()
            return False
            
        except Exception as e:
            error_msg = f"Connection error: {e}"
            log_error(error_msg, e)
            self.error_occurred.emit(error_msg)
            self._cleanup_socket()
            return False
    
    def disconnect_from_server(self):
        """
        Close connection to server.
        """
        self.logger.info("Disconnecting from server")
        self.running = False
        self.connected_flag = False
        self._cleanup_socket()

        # Clear message buffer to discard any partial messages
        self.buffer.clear()

        log_connection_event("DISCONNECTED")
        self.disconnected.emit()
    
    def send_message(self, message: str):
        """
        Send message to server.
        CRITICAL: Message must end with \\n
        
        Args:
            message: Protocol message string (should end with \\n)
        """
        if not self.connected_flag or not self.socket:
            self.logger.warning("Cannot send - not connected")
            return
        
        try:
            # Ensure message ends with newline
            if not message.endswith('\n'):
                message += '\n'
            
            # Send
            self.socket.send(message.encode('utf-8'))
            log_message_sent(message)
            
        except socket.error as e:
            error_msg = f"Send error: {e}"
            log_error(error_msg, e)
            self.error_occurred.emit(error_msg)
            self._handle_connection_lost()
            
        except Exception as e:
            error_msg = f"Unexpected send error: {e}"
            log_error(error_msg, e)
            self.error_occurred.emit(error_msg)
    
    def run(self):
        """
        Main thread loop - continuously receives data from server.
        This runs in the separate QThread.
        """
        self.running = True  # Thread is now running
        self.logger.info("NetworkClient thread started")
        
        while self.running:
            if not self.connected_flag or not self.socket:
                # Not connected, sleep briefly
                self.msleep(100)
                continue
            
            try:
                # Receive data (blocking with timeout)
                data = self.socket.recv(constants.SOCKET_RECV_BUFFER_SIZE)
                
                if not data:
                    # Server closed connection
                    self.logger.info("Server closed connection (recv returned empty)")
                    self._handle_connection_lost()
                    continue
                
                # Decode and add to buffer
                decoded = data.decode('utf-8')
                complete_messages = self.buffer.add_data(decoded)

                # ===== TIMING: Measure message processing in network thread =====
                if complete_messages:
                    batch_start = time.perf_counter()
                    self.logger.info(f"[TIMING] Processing {len(complete_messages)} message(s) in network thread")

                # Process each complete message
                for raw_message in complete_messages:
                    msg_start = time.perf_counter()
                    log_message_received(raw_message)
                    self._process_message(raw_message)
                    msg_end = time.perf_counter()
                    msg_duration = (msg_end - msg_start) * 1000  # Convert to ms

                    # Get message type for logging
                    msg_type = raw_message.split('|')[0] if '|' in raw_message else 'UNKNOWN'
                    self.logger.info(f"[TIMING] Network thread processed {msg_type}: {msg_duration:.3f} ms")

                # ===== TIMING: Total batch processing time =====
                if complete_messages:
                    batch_end = time.perf_counter()
                    batch_duration = (batch_end - batch_start) * 1000
                    self.logger.info(f"[TIMING] Total batch processing: {batch_duration:.3f} ms for {len(complete_messages)} messages")
                
            except socket.timeout:
                # Timeout is normal - just means no data received
                continue
                
            except socket.error as e:
                if self.running:  # Only log if we didn't initiate disconnect
                    error_msg = f"Socket error: {e}"
                    log_error(error_msg, e)
                    self._handle_connection_lost()
                break
                
            except Exception as e:
                if self.running:
                    error_msg = f"Receive error: {e}"
                    log_error(error_msg, e)
                    self.error_occurred.emit(error_msg)
                break
        
        self.logger.info("NetworkClient thread stopped")
    
    def _process_message(self, raw_message: str):
        """
        Parse and emit message.
        
        Args:
            raw_message: Raw protocol message string
        """
        try:
            # Parse message
            parsed = MessageProtocol.parse(raw_message)
            
            # Convert to dict for signal
            message_dict = {
                'type': parsed.type,
                'player': parsed.player_id,  # Fixed: use player_id not player
                'room': parsed.room_id,      # Fixed: use room_id not room
                'data': parsed.data,
                'raw': raw_message
            }
            
            # Emit signal
            self.message_received.emit(message_dict)
            
        except ValueError as e:
            error_msg = f"Invalid message format: {e}"
            log_error(error_msg)
            self.error_occurred.emit(error_msg)
    
    def _handle_connection_lost(self):
        """Handle unexpected connection loss"""
        if self.connected_flag:
            self.connected_flag = False
            self.running = False
            log_connection_event("CONNECTION_LOST")
            self._cleanup_socket()
            self.disconnected.emit()
    
    def _cleanup_socket(self):
        """Clean up socket resources"""
        if self.socket:
            try:
                self.socket.close()
            except Exception:
                pass
            self.socket = None
    
    def stop(self):
        """Stop the thread gracefully"""
        self.logger.info("Stopping NetworkClient")
        self.running = False
        self.disconnect_from_server()
        
        # Wait for thread to finish (max 2 seconds)
        self.wait(2000)
    
    def is_connected(self) -> bool:
        """Check if currently connected"""
        return self.connected_flag
