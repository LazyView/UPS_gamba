#!/usr/bin/env python3
"""
Test script for the network layer.
Tests NetworkClient, HeartbeatManager, and ConnectionManager.

NOTE: This requires PyQt5 and a running server on 127.0.0.1:8080
"""

import sys
sys.path.insert(0, 'src')

from PyQt5.QtWidgets import QApplication
from PyQt5.QtCore import QTimer
from network import NetworkClient, HeartbeatManager, ConnectionManager
from message import ServerMessageType


def test_connection_manager():
    """
    Test ConnectionManager with real server.
    This is an integration test that requires a running server.
    """
    print("=" * 70)
    print("TEST: ConnectionManager Integration")
    print("=" * 70)
    print("\nThis test requires a running Gamba server on 127.0.0.1:8080")
    print("Make sure the server is running before continuing!")
    print("\nPress Enter to continue, or Ctrl+C to cancel...")
    
    try:
        input()
    except KeyboardInterrupt:
        print("\nCancelled.")
        return False
    
    # Create QApplication (required for Qt)
    app = QApplication(sys.argv)
    
    # Test state
    test_state = {
        'connected': False,
        'room_joined': False,
        'errors': [],
        'messages_received': 0
    }
    
    # Create connection manager
    manager = ConnectionManager()
    
    # Connect signals
    def on_state_changed(old_state, new_state):
        print(f"  State: {old_state} → {new_state}")
    
    def on_message_received(message_dict):
        msg_type = message_dict.get('type')
        test_state['messages_received'] += 1
        print(f"  ← Message type {msg_type}: {message_dict.get('data', {})}")
        
        if msg_type == ServerMessageType.CONNECTED:
            test_state['connected'] = True
            print("  ✓ Connection successful!")
            
            # Join room after connection
            QTimer.singleShot(1000, manager.send_join_room)
        
        elif msg_type == ServerMessageType.ROOM_JOINED:
            test_state['room_joined'] = True
            print("  ✓ Room joined!")
            
            # End test after room join
            QTimer.singleShot(1000, app.quit)
        
        elif msg_type == ServerMessageType.PONG:
            print("  ✓ PONG received")
    
    def on_error(error_msg):
        print(f"  ✗ Error: {error_msg}")
        test_state['errors'].append(error_msg)
        QTimer.singleShot(1000, app.quit)
    
    manager.state_changed.connect(on_state_changed)
    manager.message_received.connect(on_message_received)
    manager.error_occurred.connect(on_error)
    
    # Start connection
    print("\nConnecting to server...")
    success = manager.connect("127.0.0.1", 8080, "TestPlayer")
    
    if not success:
        print("✗ Failed to initiate connection")
        return False
    
    # Set timeout
    QTimer.singleShot(10000, app.quit)  # 10 second timeout
    
    # Run event loop
    app.exec_()
    
    # Check results
    print("\n" + "=" * 70)
    print("TEST RESULTS")
    print("=" * 70)
    print(f"Connected: {test_state['connected']}")
    print(f"Room joined: {test_state['room_joined']}")
    print(f"Messages received: {test_state['messages_received']}")
    print(f"Errors: {len(test_state['errors'])}")
    
    if test_state['errors']:
        for error in test_state['errors']:
            print(f"  - {error}")
    
    # Cleanup
    manager.disconnect()
    
    # Evaluate
    success = (
        test_state['connected'] and
        test_state['room_joined'] and
        len(test_state['errors']) == 0
    )
    
    if success:
        print("\n✓ ConnectionManager test PASSED!")
    else:
        print("\n✗ ConnectionManager test FAILED!")
    
    return success


def main():
    """Run network layer tests"""
    print("\n" + "=" * 70)
    print("NETWORK LAYER TEST SUITE")
    print("=" * 70)
    print("\nNOTE: These tests require:")
    print("  1. PyQt5 installed (pip install PyQt5)")
    print("  2. Gamba server running on 127.0.0.1:8080")
    print("\n" + "=" * 70)
    
    try:
        # Check PyQt5
        from PyQt5.QtCore import QT_VERSION_STR
        print(f"✓ PyQt5 found (version {QT_VERSION_STR})")
    except ImportError:
        print("✗ PyQt5 not found!")
        print("  Install with: pip install PyQt5")
        return 1
    
    print()
    
    try:
        # Run tests
        success = test_connection_manager()
        
        if success:
            print("\n" + "=" * 70)
            print("✓ ALL NETWORK TESTS PASSED!")
            print("=" * 70)
            return 0
        else:
            print("\n" + "=" * 70)
            print("✗ NETWORK TESTS FAILED!")
            print("=" * 70)
            return 1
            
    except KeyboardInterrupt:
        print("\n\nTests interrupted by user.")
        return 1
    except Exception as e:
        print(f"\n✗ UNEXPECTED ERROR: {e}")
        import traceback
        traceback.print_exc()
        return 1


if __name__ == "__main__":
    sys.exit(main())
