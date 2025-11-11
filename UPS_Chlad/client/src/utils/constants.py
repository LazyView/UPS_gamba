"""
Constants and default values for the Gamba client.
"""

# ============================================================================
# NETWORK CONSTANTS
# ============================================================================

# Connection defaults (from server.conf)
DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 8080

# Timeouts (in seconds)
PING_INTERVAL = 2  # Send PING every 2 seconds
PONG_TIMEOUT = 2   # Expect PONG within 2 seconds of PING
SERVER_TIMEOUT = 6  # Server disconnects after 6s without heartbeat (3 missed pings)
RECONNECT_WINDOW = 120  # Server keeps player for 120s after disconnect

# Connection retry settings
SHORT_TERM_DISCONNECT_THRESHOLD = 60  # Automatic reconnection < 60s
RECONNECT_RETRY_INTERVAL = 2  # Try reconnecting every 2 seconds
RECONNECT_MAX_ATTEMPTS = 5  # Max auto-reconnect attempts

# Socket settings
SOCKET_RECV_BUFFER_SIZE = 4096  # 4KB receive buffer
SOCKET_TIMEOUT = 5.0  # 5 second socket timeout
MESSAGE_BUFFER_MAX_SIZE = 1024 * 1024  # 1MB max buffer size

# ============================================================================
# PROTOCOL CONSTANTS
# ============================================================================

# Message validation
INVALID_MESSAGE_THRESHOLD = 3  # Disconnect after N invalid messages
MAX_PLAYER_NAME_LENGTH = 32
VALID_NAME_CHARS = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-"

# Card format validation
VALID_RANKS = ["2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A"]
VALID_SUITS = ["H", "D", "C", "S"]
RESERVE_KEYWORD = "RESERVE"
EMPTY_PILE_MARKER = "1S"  # Special marker for empty discard pile

# ============================================================================
# GAME CONSTANTS
# ============================================================================

# Game setup
INITIAL_HAND_SIZE = 3
INITIAL_RESERVE_SIZE = 3
MAX_PLAYERS_PER_ROOM = 2  # Gamba is 2-player game

# Special card values
CARD_VALUE_TWO = 2   # Wild card
CARD_VALUE_SEVEN = 7  # Forces ≤7 play
CARD_VALUE_TEN = 10   # Burns pile
CARD_VALUE_ACE = 14   # Highest card

# Card rank values (for comparison)
CARD_VALUES = {
    "2": 2,
    "3": 3,
    "4": 4,
    "5": 5,
    "6": 6,
    "7": 7,
    "8": 8,
    "9": 9,
    "10": 10,
    "J": 11,
    "Q": 12,
    "K": 13,
    "A": 14
}

# ============================================================================
# UI CONSTANTS
# ============================================================================

# Window settings
WINDOW_TITLE = "Gamba Card Game"
WINDOW_MIN_WIDTH = 800
WINDOW_MIN_HEIGHT = 600

# Update intervals (milliseconds)
UI_UPDATE_INTERVAL = 100  # Refresh UI every 100ms if needed
STATUS_MESSAGE_DURATION = 5000  # Status messages last 5 seconds

# Colors (for card suits, if using styled widgets)
COLOR_HEARTS = "#FF0000"
COLOR_DIAMONDS = "#FF0000"
COLOR_CLUBS = "#000000"
COLOR_SPADES = "#000000"

# ============================================================================
# LOGGING CONSTANTS
# ============================================================================

# Log file settings
LOG_FILE_NAME = "gamba_client.log"
LOG_MAX_SIZE = 10 * 1024 * 1024  # 10MB max log file size
LOG_BACKUP_COUNT = 3  # Keep 3 backup log files
LOG_FORMAT = "%(asctime)s - %(name)s - %(levelname)s - %(message)s"
LOG_DATE_FORMAT = "%Y-%m-%d %H:%M:%S"

# Log levels
LOG_LEVEL_CONSOLE = "INFO"  # Console shows INFO and above
LOG_LEVEL_FILE = "DEBUG"    # File logs everything

# ============================================================================
# CONFIGURATION CONSTANTS
# ============================================================================

# Config file settings
CONFIG_FILE_NAME = "gamba_client.conf"
CONFIG_SECTION_CONNECTION = "Connection"
CONFIG_SECTION_PLAYER = "Player"
CONFIG_SECTION_UI = "UI"

# Default config values
DEFAULT_CONFIG = {
    CONFIG_SECTION_CONNECTION: {
        "host": DEFAULT_HOST,
        "port": str(DEFAULT_PORT),
        "auto_reconnect": "true"
    },
    CONFIG_SECTION_PLAYER: {
        "last_name": "",
        "remember_name": "true"
    },
    CONFIG_SECTION_UI: {
        "window_width": str(WINDOW_MIN_WIDTH),
        "window_height": str(WINDOW_MIN_HEIGHT),
        "show_debug_info": "false"
    }
}

# ============================================================================
# STATE MACHINE CONSTANTS
# ============================================================================

# Connection states (used by connection_manager.py)
STATE_DISCONNECTED = "DISCONNECTED"
STATE_CONNECTING = "CONNECTING"
STATE_CONNECTED = "CONNECTED"      # Connected but not in room
STATE_IN_LOBBY = "IN_LOBBY"        # In room selection
STATE_IN_ROOM = "IN_ROOM"          # In room, waiting for game
STATE_IN_GAME = "IN_GAME"          # Game active
STATE_RECONNECTING = "RECONNECTING"  # Attempting reconnection

# ============================================================================
# ERROR MESSAGES
# ============================================================================

ERROR_CONNECTION_REFUSED = "Could not connect to server. Is it running?"
ERROR_CONNECTION_LOST = "Connection to server lost."
ERROR_INVALID_NAME = f"Player name must be 1-{MAX_PLAYER_NAME_LENGTH} characters (alphanumeric, _, -)."
ERROR_NAME_TAKEN = "Player name already taken."
ERROR_NOT_YOUR_TURN = "It's not your turn!"
ERROR_INVALID_CARD_PLAY = "Invalid card play according to game rules."
ERROR_SERVER_TIMEOUT = "Server connection timed out."
ERROR_RECONNECT_FAILED = "Could not reconnect to server."

# ============================================================================
# SUCCESS MESSAGES
# ============================================================================

MSG_CONNECTED = "Connected to server successfully!"
MSG_JOINED_ROOM = "Joined room successfully!"
MSG_GAME_STARTED = "Game started! Good luck!"
MSG_YOUR_TURN = "Your turn!"
MSG_OPPONENT_TURN = "Opponent's turn"
MSG_GAME_WON = "You won! 🎉"
MSG_GAME_LOST = "You lost. Better luck next time!"
MSG_RECONNECTED = "Reconnected successfully!"
