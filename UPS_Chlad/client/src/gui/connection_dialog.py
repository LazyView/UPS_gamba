"""
Connection dialog - initial screen for connecting to server.
"""

from PyQt5.QtWidgets import (
    QDialog, QVBoxLayout, QHBoxLayout, QLabel, 
    QLineEdit, QPushButton, QGroupBox, QFormLayout,
    QMessageBox
)
from PyQt5.QtCore import Qt, pyqtSignal
from PyQt5.QtGui import QIntValidator

from utils import Config, constants


class ConnectionDialog(QDialog):
    """
    Dialog for entering connection details.
    
    Allows user to enter:
    - Server IP address
    - Server port
    - Player name
    
    Emits connect_requested signal with (host, port, name) when user clicks Connect.
    
    Signals:
        connect_requested(str, int, str): Emitted when user clicks Connect
    """
    
    # Signal emitted when user wants to connect
    connect_requested = pyqtSignal(str, int, str)  # (host, port, name)
    reconnect_requested = pyqtSignal(str, str, str) # (host, port, name)
    
    def __init__(self, parent=None):
        """
        Initialize connection dialog.
        
        Args:
            parent: Parent widget
        """
        super().__init__(parent)
        
        self.config = Config()
        self.config.load()
        
        self.setWindowTitle("Connect to Gamba Server")
        self.setModal(True)
        self.setFixedSize(400, 300)
        
        self._setup_ui()
        self._load_saved_values()
        
    def _setup_ui(self):
        """Setup the user interface"""
        layout = QVBoxLayout()
        
        # Title
        title = QLabel("Gamba Card Game")
        title.setAlignment(Qt.AlignCenter)
        title.setStyleSheet("font-size: 18px; font-weight: bold; margin: 10px;")
        layout.addWidget(title)
        
        # Connection settings group
        connection_group = QGroupBox("Server Connection")
        connection_layout = QFormLayout()
        
        # IP Address input
        self.host_input = QLineEdit()
        self.host_input.setPlaceholderText("127.0.0.1")
        self.host_input.setToolTip("Server IP address or hostname")
        connection_layout.addRow("Server Address:", self.host_input)
        
        # Port input
        self.port_input = QLineEdit()
        self.port_input.setPlaceholderText(str(constants.DEFAULT_PORT))
        self.port_input.setValidator(QIntValidator(1, 65535))  # Valid port range
        self.port_input.setToolTip("Server port (1-65535)")
        connection_layout.addRow("Port:", self.port_input)
        
        connection_group.setLayout(connection_layout)
        layout.addWidget(connection_group)
        
        # Player settings group
        player_group = QGroupBox("Player Information")
        player_layout = QFormLayout()
        
        # Player name input
        self.name_input = QLineEdit()
        self.name_input.setPlaceholderText("Enter your name")
        self.name_input.setMaxLength(32)  # Server limit
        self.name_input.setToolTip("Your player name (alphanumeric, _, - only)")
        player_layout.addRow("Player Name:", self.name_input)
        
        player_group.setLayout(player_layout)
        layout.addWidget(player_group)
        
        # Status label
        self.status_label = QLabel("")
        self.status_label.setAlignment(Qt.AlignCenter)
        self.status_label.setStyleSheet("color: gray; font-style: italic;")
        layout.addWidget(self.status_label)
        
        # Buttons
        button_layout = QHBoxLayout()
        
        self.connect_button = QPushButton("Connect")
        self.connect_button.setDefault(True)  # Enter key triggers this
        self.connect_button.clicked.connect(self._on_connect_clicked)
        self.connect_button.setStyleSheet("""
            QPushButton {
                background-color: #4CAF50;
                color: white;
                padding: 8px;
                font-weight: bold;
                border-radius: 4px;
            }
            QPushButton:hover {
                background-color: #45a049;
            }
            QPushButton:disabled {
                background-color: #cccccc;
            }
        """)
        
        self.cancel_button = QPushButton("Exit")
        self.cancel_button.clicked.connect(self.reject)
        
        button_layout.addWidget(self.cancel_button)
        button_layout.addWidget(self.connect_button)
        
        layout.addLayout(button_layout)
        layout.addStretch()
        
        self.setLayout(layout)
        
        # Enable Enter key on all inputs
        self.host_input.returnPressed.connect(self._on_connect_clicked)
        self.port_input.returnPressed.connect(self._on_connect_clicked)
        self.name_input.returnPressed.connect(self._on_connect_clicked)
    
    def _load_saved_values(self):
        """Load previously saved values from config"""
        # Load host
        host = self.config.get_connection_host()
        if host:
            self.host_input.setText(host)
        
        # Load port
        port = self.config.get_connection_port()
        if port:
            self.port_input.setText(str(port))
        
        # Load name if remember is enabled
        if self.config.get_remember_name():
            name = self.config.get_last_player_name()
            if name:
                self.name_input.setText(name)
    
    def _validate_inputs(self) -> tuple[bool, str]:
        """
        Validate user inputs.
        
        Returns:
            Tuple of (is_valid, error_message)
        """
        # Validate host
        host = self.host_input.text().strip()
        if not host:
            return False, "Please enter a server address"
        
        # Validate port
        port_text = self.port_input.text().strip()
        if not port_text:
            return False, "Please enter a port number"
        
        try:
            port = int(port_text)
            if port < 1 or port > 65535:
                return False, "Port must be between 1 and 65535"
        except ValueError:
            return False, "Port must be a number"
        
        # Validate name
        name = self.name_input.text().strip()
        if not name:
            return False, "Please enter your player name"
        
        if len(name) > 32:
            return False, "Name must be 32 characters or less"
        
        # Check for valid characters (alphanumeric, underscore, hyphen)
        if not all(c.isalnum() or c in ('_', '-') for c in name):
            return False, "Name can only contain letters, numbers, _ and -"
        
        return True, ""
    
    def _save_values(self):
        """Save current values to config"""
        host = self.host_input.text().strip()
        port = int(self.port_input.text().strip())
        name = self.name_input.text().strip()
        
        self.config.set_connection_host(host)
        self.config.set_connection_port(port)
        self.config.set_last_player_name(name)
        self.config.set_remember_name(True)
        self.config.save()
    
    def _on_connect_clicked(self):
        """Handle connect button click"""
        # Validate inputs
        valid, error = self._validate_inputs()
        
        if not valid:
            self.show_error(error)
            return
        
        # Get values
        host = self.host_input.text().strip()
        port = int(self.port_input.text().strip())
        name = self.name_input.text().strip()
        
        # Save values
        self._save_values()
        
        # Disable inputs while connecting
        self.set_connecting(True)
        
        # Emit signal
        self.connect_requested.emit(host, port, name)
    
    def set_connecting(self, connecting: bool):
        """
        Set UI state for connecting.
        
        Args:
            connecting: True if currently connecting
        """
        self.host_input.setEnabled(not connecting)
        self.port_input.setEnabled(not connecting)
        self.name_input.setEnabled(not connecting)
        self.connect_button.setEnabled(not connecting)
        
        if connecting:
            self.status_label.setText("Connecting to server...")
            self.status_label.setStyleSheet("color: blue; font-style: italic;")
        else:
            self.status_label.setText("")
    
    def show_error(self, message: str):
        """
        Show error message.

        Args:
            message: Error message to display
        """
        # Re-enable UI first (this clears status label)
        self.set_connecting(False)
        # Then set error message (so it doesn't get cleared)
        self.status_label.setText(f"Error: {message}")
        self.status_label.setStyleSheet("color: red; font-weight: bold;")

        # Also show as non-blocking message box for critical errors
        if "refused" in message.lower() or "timeout" in message.lower() or "full" in message.lower():
            msg_box = QMessageBox(self)
            msg_box.setWindowTitle("Connection Error")
            msg_box.setText(message)
            msg_box.setIcon(QMessageBox.Critical)
            msg_box.setStandardButtons(QMessageBox.Ok)
            msg_box.setWindowModality(Qt.NonModal)  # Make it truly non-blocking
            msg_box.setAttribute(Qt.WA_DeleteOnClose)  # Auto-cleanup when closed
            msg_box.show()  # Non-blocking
    
    def show_success(self):
        """Show success message and close dialog"""
        self.status_label.setText("Connected!")
        self.status_label.setStyleSheet("color: green; font-weight: bold;")
        
        # Close dialog after brief delay
        from PyQt5.QtCore import QTimer
        QTimer.singleShot(500, self.accept)
    
    def get_values(self) -> tuple[str, int, str]:
        """
        Get current input values.
        
        Returns:
            Tuple of (host, port, name)
        """
        host = self.host_input.text().strip()
        port = int(self.port_input.text().strip())
        name = self.name_input.text().strip()
        return host, port, name
