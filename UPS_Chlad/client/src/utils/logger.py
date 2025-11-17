"""
Logging system for the Gamba client.
Provides file and console logging with proper formatting.
"""

import logging
import logging.handlers
import os
from typing import Optional

from .constants import (
    LOG_FILE_NAME,
    LOG_MAX_SIZE,
    LOG_BACKUP_COUNT,
    LOG_FORMAT,
    LOG_DATE_FORMAT,
    LOG_LEVEL_CONSOLE,
    LOG_LEVEL_FILE
)


class GameLogger:
    """
    Centralized logging system for the Gamba client.
    
    Features:
    - File logging with rotation (keeps old logs)
    - Console logging
    - Separate log levels for file vs console
    - Special methods for network message logging
    - State transition logging
    
    Usage:
        logger = GameLogger.get_logger()
        logger.info("Application started")
        logger.log_message_sent("0|||name=Alice")
        logger.log_state_change("DISCONNECTED", "CONNECTED")
    """
    
    _instance: Optional['GameLogger'] = None
    _logger: Optional[logging.Logger] = None
    
    def __init__(self, log_dir: str = "logs", log_file: str = LOG_FILE_NAME):
        """
        Initialize the logger.
        
        Args:
            log_dir: Directory for log files (created if doesn't exist)
            log_file: Name of the log file
        """
        if GameLogger._logger is not None:
            return  # Already initialized
        
        # Create log directory if it doesn't exist
        if not os.path.exists(log_dir):
            os.makedirs(log_dir)
        
        log_path = os.path.join(log_dir, log_file)
        
        # Create logger
        self._logger = logging.getLogger("GambaClient")
        self._logger.setLevel(logging.INFO)  # Capture everything
        
        # Remove existing handlers (in case of re-initialization)
        self._logger.handlers.clear()
        
        # File handler with rotation
        file_handler = logging.handlers.RotatingFileHandler(
            log_path,
            maxBytes=LOG_MAX_SIZE,
            backupCount=LOG_BACKUP_COUNT,
            encoding='utf-8'
        )
        file_handler.setLevel(self._get_log_level(LOG_LEVEL_FILE))
        file_formatter = logging.Formatter(LOG_FORMAT, LOG_DATE_FORMAT)
        file_handler.setFormatter(file_formatter)
        self._logger.addHandler(file_handler)
        
        # Console handler
        # console_handler = logging.StreamHandler()
        # console_handler.setLevel(self._get_log_level(LOG_LEVEL_CONSOLE))
        # console_formatter = logging.Formatter(LOG_FORMAT, LOG_DATE_FORMAT)
        # console_handler.setFormatter(console_formatter)
        # self._logger.addHandler(console_handler)
        
        # Store reference
        GameLogger._logger = self._logger
        
        self._logger.info("=" * 70)
        self._logger.info("Gamba Client Logger Initialized")
        self._logger.info("=" * 70)
    
    @staticmethod
    def _get_log_level(level_str: str) -> int:
        """Convert string log level to logging constant"""
        levels = {
            "DEBUG": logging.DEBUG,
            "INFO": logging.INFO,
            "WARNING": logging.WARNING,
            "ERROR": logging.ERROR,
            "CRITICAL": logging.CRITICAL
        }
        return levels.get(level_str.upper(), logging.INFO)
    
    @classmethod
    def get_logger(cls) -> logging.Logger:
        """
        Get the logger instance (singleton pattern).
        Creates logger if it doesn't exist.
        
        Returns:
            logging.Logger instance
        """
        if cls._logger is None:
            cls()  # Initialize
        return cls._logger
    
    @classmethod
    def log_message_sent(cls, message: str):
        """
        Log an outgoing network message.
        
        Args:
            message: Raw message string (without newline)
        """
        logger = cls.get_logger()
        # Remove newline for cleaner logging
        clean_msg = message.rstrip('\n')
        logger.debug(f"→ SEND: {clean_msg}")
    
    @classmethod
    def log_message_received(cls, message: str):
        """
        Log an incoming network message.
        
        Args:
            message: Raw message string (without newline)
        """
        logger = cls.get_logger()
        clean_msg = message.rstrip('\n')
        logger.debug(f"← RECV: {clean_msg}")
    
    @classmethod
    def log_state_change(cls, old_state: str, new_state: str, context: str = ""):
        """
        Log a state transition.
        
        Args:
            old_state: Previous state
            new_state: New state
            context: Optional context information
        """
        logger = cls.get_logger()
        if context:
            logger.info(f"State: {old_state} → {new_state} ({context})")
        else:
            logger.info(f"State: {old_state} → {new_state}")
    
    @classmethod
    def log_connection_event(cls, event: str, details: str = ""):
        """
        Log a connection-related event.
        
        Args:
            event: Event type (e.g., "CONNECTED", "DISCONNECTED", "TIMEOUT")
            details: Additional details
        """
        logger = cls.get_logger()
        if details:
            logger.info(f"Connection: {event} - {details}")
        else:
            logger.info(f"Connection: {event}")
    
    @classmethod
    def log_game_event(cls, event: str, details: str = ""):
        """
        Log a game-related event.
        
        Args:
            event: Event type (e.g., "GAME_STARTED", "CARD_PLAYED", "GAME_OVER")
            details: Additional details
        """
        logger = cls.get_logger()
        if details:
            logger.info(f"Game: {event} - {details}")
        else:
            logger.info(f"Game: {event}")
    
    @classmethod
    def log_error(cls, error: str, exception: Optional[Exception] = None):
        """
        Log an error with optional exception details.
        
        Args:
            error: Error message
            exception: Optional exception object
        """
        logger = cls.get_logger()
        if exception:
            logger.error(f"{error}: {exception}", exc_info=True)
        else:
            logger.error(error)
    
    @classmethod
    def log_validation_error(cls, message: str, reason: str):
        """
        Log a message validation error.
        
        Args:
            message: The invalid message
            reason: Why it's invalid
        """
        logger = cls.get_logger()
        logger.warning(f"Invalid message: {reason} | Message: {message}")


# Convenience functions for direct import
def get_logger() -> logging.Logger:
    """Get the game logger instance"""
    return GameLogger.get_logger()


def log_message_sent(message: str):
    """Log outgoing message"""
    GameLogger.log_message_sent(message)


def log_message_received(message: str):
    """Log incoming message"""
    GameLogger.log_message_received(message)


def log_state_change(old_state: str, new_state: str, context: str = ""):
    """Log state transition"""
    GameLogger.log_state_change(old_state, new_state, context)


def log_connection_event(event: str, details: str = ""):
    """Log connection event"""
    GameLogger.log_connection_event(event, details)


def log_game_event(event: str, details: str = ""):
    """Log game event"""
    GameLogger.log_game_event(event, details)


def log_error(error: str, exception: Optional[Exception] = None):
    """Log error"""
    GameLogger.log_error(error, exception)


def log_validation_error(message: str, reason: str):
    """Log validation error"""
    GameLogger.log_validation_error(message, reason)


# Initialize logger on module import
GameLogger()
