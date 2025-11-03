#!/usr/bin/env python3
"""
Test script for the utils layer.
Tests logger, config, and constants.
"""

import sys
import os
sys.path.insert(0, 'src')

from utils import (
    get_logger,
    log_message_sent,
    log_message_received,
    log_state_change,
    log_connection_event,
    log_game_event,
    log_error,
    Config,
    constants
)


def test_logger():
    """Test logging functionality"""
    print("=" * 70)
    print("TEST: Logger")
    print("=" * 70)
    
    logger = get_logger()
    
    # Test basic logging
    logger.info("Test INFO message")
    logger.debug("Test DEBUG message")
    logger.warning("Test WARNING message")
    logger.error("Test ERROR message")
    
    # Test specialized logging
    log_message_sent("0|||name=TestPlayer")
    log_message_received("100|TestPlayer||name=TestPlayer|status=success")
    log_state_change("DISCONNECTED", "CONNECTED", "TCP connection established")
    log_connection_event("CONNECTED", "127.0.0.1:8080")
    log_game_event("CARD_PLAYED", "2H,2D")
    
    # Test error logging
    try:
        raise ValueError("Test exception")
    except Exception as e:
        log_error("Test error with exception", e)
    
    print("✓ Logger test completed")
    print("✓ Check logs/gamba_client.log for output")
    print()


def test_config():
    """Test configuration management"""
    print("=" * 70)
    print("TEST: Config")
    print("=" * 70)
    
    # Create config
    config = Config("test_config.conf")
    
    # Test getters (should return defaults)
    print(f"Default host: {config.get_connection_host()}")
    print(f"Default port: {config.get_connection_port()}")
    print(f"Default player name: {config.get_last_player_name()}")
    
    assert config.get_connection_host() == "127.0.0.1"
    assert config.get_connection_port() == 8080
    
    # Test setters
    config.set_connection_host("192.168.1.100")
    config.set_connection_port(9999)
    config.set_last_player_name("TestPlayer")
    config.set_remember_name(True)
    config.set_window_size(1024, 768)
    
    # Verify setters worked
    assert config.get_connection_host() == "192.168.1.100"
    assert config.get_connection_port() == 9999
    assert config.get_last_player_name() == "TestPlayer"
    assert config.get_remember_name() == True
    assert config.get_window_size() == (1024, 768)
    
    print("✓ All getters/setters work correctly")
    
    # Test save/load
    save_success = config.save()
    assert save_success, "Failed to save config"
    print("✓ Config saved successfully")
    
    # Create new config and load
    config2 = Config("test_config.conf")
    load_success = config2.load()
    assert load_success, "Failed to load config"
    print("✓ Config loaded successfully")
    
    # Verify loaded values
    assert config2.get_connection_host() == "192.168.1.100"
    assert config2.get_connection_port() == 9999
    assert config2.get_last_player_name() == "TestPlayer"
    print("✓ Loaded values match saved values")
    
    # Cleanup
    if os.path.exists("test_config.conf"):
        os.remove("test_config.conf")
        print("✓ Test config file cleaned up")
    
    print("✓ Config test completed")
    print()


def test_constants():
    """Test that constants are accessible"""
    print("=" * 70)
    print("TEST: Constants")
    print("=" * 70)
    
    # Test network constants
    print(f"DEFAULT_HOST: {constants.DEFAULT_HOST}")
    print(f"DEFAULT_PORT: {constants.DEFAULT_PORT}")
    print(f"PING_INTERVAL: {constants.PING_INTERVAL}s")
    print(f"SERVER_TIMEOUT: {constants.SERVER_TIMEOUT}s")
    
    assert constants.DEFAULT_HOST == "127.0.0.1"
    assert constants.DEFAULT_PORT == 8080
    assert constants.PING_INTERVAL == 30
    
    # Test game constants
    print(f"INITIAL_HAND_SIZE: {constants.INITIAL_HAND_SIZE}")
    print(f"INITIAL_RESERVE_SIZE: {constants.INITIAL_RESERVE_SIZE}")
    print(f"MAX_PLAYERS_PER_ROOM: {constants.MAX_PLAYERS_PER_ROOM}")
    
    assert constants.INITIAL_HAND_SIZE == 3
    assert constants.INITIAL_RESERVE_SIZE == 3
    
    # Test card constants
    print(f"VALID_RANKS: {constants.VALID_RANKS}")
    print(f"VALID_SUITS: {constants.VALID_SUITS}")
    
    assert "2" in constants.VALID_RANKS
    assert "A" in constants.VALID_RANKS
    assert "H" in constants.VALID_SUITS
    
    # Test state constants
    print(f"STATE_DISCONNECTED: {constants.STATE_DISCONNECTED}")
    print(f"STATE_IN_GAME: {constants.STATE_IN_GAME}")
    
    # Test messages
    print(f"MSG_CONNECTED: {constants.MSG_CONNECTED}")
    print(f"ERROR_CONNECTION_REFUSED: {constants.ERROR_CONNECTION_REFUSED}")
    
    print("✓ All constants accessible")
    print("✓ Constants test completed")
    print()


def main():
    """Run all utils tests"""
    print("\n" + "=" * 70)
    print("UTILS LAYER TEST SUITE")
    print("=" * 70 + "\n")
    
    try:
        test_constants()
        test_logger()
        test_config()
        
        print("=" * 70)
        print("✓ ALL UTILS TESTS PASSED!")
        print("=" * 70)
        return 0
        
    except AssertionError as e:
        print(f"\n✗ TEST FAILED: {e}")
        return 1
    except Exception as e:
        print(f"\n✗ UNEXPECTED ERROR: {e}")
        import traceback
        traceback.print_exc()
        return 1


if __name__ == "__main__":
    sys.exit(main())
