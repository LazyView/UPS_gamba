"""
Heartbeat manager for keeping connection alive.
Sends PING every 30 seconds and detects timeouts.
"""

from PyQt5.QtCore import QObject, QTimer, pyqtSignal
from datetime import datetime, timedelta

from message import MessageProtocol, ClientMessageType, ServerMessageType
from utils import get_logger, log_connection_event, constants


class HeartbeatManager(QObject):
    """
    Manages PING/PONG heartbeat to keep connection alive.
    
    The server disconnects clients that don't send anything for 60 seconds.
    We send PING every 30 seconds to stay alive.
    We also track PONG responses to detect if server stopped responding.
    
    Signals:
    - timeout_detected: Emitted when server doesn't respond to PING
    - ping_sent: Emitted when PING is sent (for debugging/UI)
    - pong_received: Emitted when PONG is received (for debugging/UI)
    
    Usage:
        heartbeat = HeartbeatManager()
        heartbeat.timeout_detected.connect(on_timeout)
        heartbeat.start(send_message_func)
    """
    
    # Signals
    timeout_detected = pyqtSignal()  # Server not responding
    ping_sent = pyqtSignal()  # PING sent
    pong_received = pyqtSignal()  # PONG received
    
    def __init__(self):
        """Initialize heartbeat manager"""
        super().__init__()
        
        self.logger = get_logger()
        
        # Timer for sending PING
        self.ping_timer = QTimer()
        self.ping_timer.timeout.connect(self._send_ping)
        
        # Timer for detecting PONG timeout
        self.pong_timer = QTimer()
        self.pong_timer.timeout.connect(self._check_pong_timeout)
        
        # State
        self.running = False
        self.send_message_callback = None
        self.last_ping_time: datetime = None
        self.waiting_for_pong = False
        
        self.logger.info("HeartbeatManager initialized")
    
    def start(self, send_message_callback):
        """
        Start heartbeat.
        
        Args:
            send_message_callback: Function to call to send messages (e.g., network_client.send_message)
        """
        if self.running:
            self.logger.warning("Heartbeat already running")
            return
        
        self.send_message_callback = send_message_callback
        self.running = True
        
        # Start PING timer (send every 30 seconds)
        self.ping_timer.start(constants.PING_INTERVAL * 1000)  # Convert to milliseconds
        
        # Send first PING immediately
        self._send_ping()
        
        self.logger.info(f"Heartbeat started (PING every {constants.PING_INTERVAL}s)")
        log_connection_event("HEARTBEAT_STARTED")
    
    def stop(self):
        """Stop heartbeat"""
        if not self.running:
            return
        
        self.running = False
        self.ping_timer.stop()
        self.pong_timer.stop()
        self.waiting_for_pong = False
        
        self.logger.info("Heartbeat stopped")
        log_connection_event("HEARTBEAT_STOPPED")
    
    def on_pong_received(self):
        """
        Call this when PONG message is received from server.
        """
        if not self.waiting_for_pong:
            self.logger.warning("Received PONG but wasn't waiting for one")
            return
        
        # Calculate round-trip time
        if self.last_ping_time:
            rtt = (datetime.now() - self.last_ping_time).total_seconds()
            self.logger.debug(f"PONG received (RTT: {rtt:.3f}s)")
        
        self.waiting_for_pong = False
        self.pong_timer.stop()
        self.pong_received.emit()
    
    def _send_ping(self):
        """Send PING message to server"""
        if not self.running or not self.send_message_callback:
            return
        
        # Build PING message
        ping_message = MessageProtocol.build(ClientMessageType.PING)
        
        # Send
        self.send_message_callback(ping_message)
        
        # Track state
        self.last_ping_time = datetime.now()
        self.waiting_for_pong = True
        
        # Start timeout timer (expect PONG within PONG_TIMEOUT seconds)
        self.pong_timer.start(constants.PONG_TIMEOUT * 1000)
        
        self.logger.debug("PING sent")
        self.ping_sent.emit()
    
    def _check_pong_timeout(self):
        """Check if PONG timeout occurred"""
        if self.waiting_for_pong:
            self.logger.warning(f"PONG timeout - no response within {constants.PONG_TIMEOUT}s")
            log_connection_event("PONG_TIMEOUT", f"No response within {constants.PONG_TIMEOUT}s")
            
            self.waiting_for_pong = False
            self.stop()  # Stop heartbeat
            self.timeout_detected.emit()
    
    def is_running(self) -> bool:
        """Check if heartbeat is running"""
        return self.running
    
    def is_waiting_for_pong(self) -> bool:
        """Check if currently waiting for PONG response"""
        return self.waiting_for_pong
