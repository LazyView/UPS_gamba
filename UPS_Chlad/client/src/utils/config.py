"""
Configuration management for the Gamba client.
Saves and loads user settings (last used IP/port/name, UI preferences, etc.).
"""

import configparser
import os
from typing import Optional, Dict, Any

from .constants import (
    CONFIG_FILE_NAME,
    CONFIG_SECTION_CONNECTION,
    CONFIG_SECTION_PLAYER,
    CONFIG_SECTION_UI,
    DEFAULT_CONFIG,
    DEFAULT_HOST,
    DEFAULT_PORT
)
from .logger import get_logger


class Config:
    """
    Configuration manager for saving/loading client settings.
    
    Uses INI file format for human-readable config.
    Settings are saved in the client directory.
    
    Usage:
        config = Config()
        config.load()
        
        host = config.get_connection_host()
        config.set_connection_host("192.168.1.100")
        config.save()
    """
    
    def __init__(self, config_file: str = CONFIG_FILE_NAME):
        """
        Initialize config manager.
        
        Args:
            config_file: Path to config file
        """
        self.config_file = config_file
        self.config = configparser.ConfigParser()
        self.logger = get_logger()
        
        # Initialize with defaults
        self._set_defaults()
    
    def _set_defaults(self):
        """Set default configuration values"""
        for section, options in DEFAULT_CONFIG.items():
            if not self.config.has_section(section):
                self.config.add_section(section)
            for key, value in options.items():
                self.config.set(section, key, value)
    
    def load(self) -> bool:
        """
        Load configuration from file.
        
        Returns:
            True if loaded successfully, False otherwise
        """
        if not os.path.exists(self.config_file):
            self.logger.info(f"Config file not found: {self.config_file} - using defaults")
            return False
        
        try:
            self.config.read(self.config_file, encoding='utf-8')
            self.logger.info(f"Configuration loaded from {self.config_file}")
            return True
        except Exception as e:
            self.logger.error(f"Failed to load config: {e}")
            self._set_defaults()  # Restore defaults
            return False
    
    def save(self) -> bool:
        """
        Save configuration to file.
        
        Returns:
            True if saved successfully, False otherwise
        """
        try:
            with open(self.config_file, 'w', encoding='utf-8') as f:
                self.config.write(f)
            self.logger.info(f"Configuration saved to {self.config_file}")
            return True
        except Exception as e:
            self.logger.error(f"Failed to save config: {e}")
            return False
    
    # ========================================================================
    # CONNECTION SETTINGS
    # ========================================================================
    
    def get_connection_host(self) -> str:
        """Get server host"""
        return self.config.get(CONFIG_SECTION_CONNECTION, "host", fallback=DEFAULT_HOST)
    
    def set_connection_host(self, host: str):
        """Set server host"""
        self.config.set(CONFIG_SECTION_CONNECTION, "host", host)
    
    def get_connection_port(self) -> int:
        """Get server port"""
        try:
            return self.config.getint(CONFIG_SECTION_CONNECTION, "port", fallback=DEFAULT_PORT)
        except ValueError:
            return DEFAULT_PORT
    
    def set_connection_port(self, port: int):
        """Set server port"""
        self.config.set(CONFIG_SECTION_CONNECTION, "port", str(port))
    
    def get_auto_reconnect(self) -> bool:
        """Get auto-reconnect setting"""
        return self.config.getboolean(CONFIG_SECTION_CONNECTION, "auto_reconnect", fallback=True)
    
    def set_auto_reconnect(self, enabled: bool):
        """Set auto-reconnect setting"""
        self.config.set(CONFIG_SECTION_CONNECTION, "auto_reconnect", str(enabled).lower())
    
    # ========================================================================
    # PLAYER SETTINGS
    # ========================================================================
    
    def get_last_player_name(self) -> str:
        """Get last used player name"""
        return self.config.get(CONFIG_SECTION_PLAYER, "last_name", fallback="")
    
    def set_last_player_name(self, name: str):
        """Set last used player name"""
        self.config.set(CONFIG_SECTION_PLAYER, "last_name", name)
    
    def get_remember_name(self) -> bool:
        """Get whether to remember player name"""
        return self.config.getboolean(CONFIG_SECTION_PLAYER, "remember_name", fallback=True)
    
    def set_remember_name(self, enabled: bool):
        """Set whether to remember player name"""
        self.config.set(CONFIG_SECTION_PLAYER, "remember_name", str(enabled).lower())
    
    # ========================================================================
    # UI SETTINGS
    # ========================================================================
    
    def get_window_size(self) -> tuple[int, int]:
        """
        Get window size.
        
        Returns:
            (width, height) tuple
        """
        try:
            width = self.config.getint(CONFIG_SECTION_UI, "window_width", fallback=800)
            height = self.config.getint(CONFIG_SECTION_UI, "window_height", fallback=600)
            return (width, height)
        except ValueError:
            return (800, 600)
    
    def set_window_size(self, width: int, height: int):
        """Set window size"""
        self.config.set(CONFIG_SECTION_UI, "window_width", str(width))
        self.config.set(CONFIG_SECTION_UI, "window_height", str(height))
    
    def get_show_debug_info(self) -> bool:
        """Get whether to show debug information in UI"""
        return self.config.getboolean(CONFIG_SECTION_UI, "show_debug_info", fallback=False)
    
    def set_show_debug_info(self, enabled: bool):
        """Set whether to show debug information in UI"""
        self.config.set(CONFIG_SECTION_UI, "show_debug_info", str(enabled).lower())
    
    # ========================================================================
    # GENERIC GETTERS/SETTERS
    # ========================================================================
    
    def get(self, section: str, option: str, fallback: Any = None) -> Optional[str]:
        """
        Get arbitrary config value.
        
        Args:
            section: Config section
            option: Config option
            fallback: Default value if not found
            
        Returns:
            Config value or fallback
        """
        return self.config.get(section, option, fallback=fallback)
    
    def set(self, section: str, option: str, value: str):
        """
        Set arbitrary config value.
        
        Args:
            section: Config section
            option: Config option
            value: Value to set
        """
        if not self.config.has_section(section):
            self.config.add_section(section)
        self.config.set(section, option, value)
    
    def to_dict(self) -> Dict[str, Dict[str, str]]:
        """
        Convert config to dictionary.
        
        Returns:
            Dictionary of {section: {option: value}}
        """
        result = {}
        for section in self.config.sections():
            result[section] = dict(self.config.items(section))
        return result
    
    def __repr__(self):
        return f"Config(file={self.config_file}, sections={list(self.config.sections())})"
