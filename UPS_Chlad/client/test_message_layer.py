#!/usr/bin/env python3
"""
Simple test script for the message layer.
Tests parsing, building, and buffering.
"""

import sys
sys.path.insert(0, 'src')

from message import (
    MessageBuffer, 
    MessageProtocol, 
    ClientMessageType, 
    ServerMessageType,
    MessageValidator
)


def test_message_building():
    """Test building messages"""
    print("=== Testing Message Building ===")
    
    # Test CONNECT message
    msg = MessageProtocol.build(ClientMessageType.CONNECT, name="Alice")
    print(f"CONNECT: {repr(msg)}")
    assert msg == "0|||name=Alice\n", f"Expected '0|||name=Alice\\n', got {repr(msg)}"
    
    # Test PLAY_CARDS message
    msg = MessageProtocol.build(ClientMessageType.PLAY_CARDS, cards="2H,2D")
    print(f"PLAY_CARDS: {repr(msg)}")
    assert msg == "7|||cards=2H,2D\n"
    
    # Test PING message
    msg = MessageProtocol.build(ClientMessageType.PING)
    print(f"PING: {repr(msg)}")
    assert msg == "4|||\n"
    
    print("✓ Building tests passed!\n")


def test_message_parsing():
    """Test parsing messages"""
    print("=== Testing Message Parsing ===")
    
    # Test CONNECTED message
    raw = "100|Alice||name=Alice|status=success"
    msg = MessageProtocol.parse(raw)
    print(f"Parsed CONNECTED: {msg}")
    assert msg.type == ServerMessageType.CONNECTED
    assert msg.player_id == "Alice"
    assert msg.data['name'] == "Alice"
    assert msg.data['status'] == "success"
    
    # Test GAME_STATE message
    raw = "106|Alice|ROOM_1|hand=2H,AS|your_turn=true|top_card=5H"
    msg = MessageProtocol.parse(raw)
    print(f"Parsed GAME_STATE: {msg}")
    assert msg.type == ServerMessageType.GAME_STATE
    assert msg.player_id == "Alice"
    assert msg.room_id == "ROOM_1"
    assert msg.data['hand'] == "2H,AS"
    assert msg.data['your_turn'] == "true"
    
    # Test message with equals in value
    raw = "100|Alice||formula=a=b+c"
    msg = MessageProtocol.parse(raw)
    print(f"Parsed with = in value: {msg}")
    assert msg.data['formula'] == "a=b+c", f"Expected 'a=b+c', got {msg.data['formula']}"
    
    print("✓ Parsing tests passed!\n")


def test_message_buffer():
    """Test message buffering"""
    print("=== Testing Message Buffer ===")
    
    buffer = MessageBuffer()
    
    # Test 1: Complete message
    messages = buffer.add_data("100|Alice||name=Alice\n")
    print(f"Test 1 - Complete message: {messages}")
    assert len(messages) == 1
    assert messages[0] == "100|Alice||name=Alice"
    
    # Test 2: Partial message
    messages = buffer.add_data("100|Bob||name=")
    print(f"Test 2 - Partial (first part): {messages}")
    assert len(messages) == 0  # No complete message yet
    
    messages = buffer.add_data("Bob\n")
    print(f"Test 2 - Partial (second part): {messages}")
    assert len(messages) == 1
    assert messages[0] == "100|Bob||name=Bob"
    
    # Test 3: Multiple messages at once
    messages = buffer.add_data("101|Alice|R1||status=ok\n102|Alice||status=left\n")
    print(f"Test 3 - Multiple messages: {messages}")
    assert len(messages) == 2
    assert messages[0] == "101|Alice|R1||status=ok"
    assert messages[1] == "102|Alice||status=left"
    
    # Test 4: Mixed (complete + partial)
    messages = buffer.add_data("100|Test||data=value\n101|Test")
    print(f"Test 4 - Mixed (part 1): {messages}")
    assert len(messages) == 1
    assert messages[0] == "100|Test||data=value"
    assert buffer.has_data()  # Still has incomplete data
    
    messages = buffer.add_data("||more=data\n")
    print(f"Test 4 - Mixed (part 2): {messages}")
    assert len(messages) == 1
    assert messages[0] == "101|Test||more=data"
    
    print("✓ Buffer tests passed!\n")


def test_message_validator():
    """Test message validation"""
    print("=== Testing Message Validator ===")
    
    validator = MessageValidator(threshold=3)
    
    # Test valid server message type
    msg = MessageProtocol.parse("100|Alice||name=Alice")
    is_valid, error = validator.validate_incoming(msg, "DISCONNECTED")
    print(f"Valid message: is_valid={is_valid}, error={error}")
    assert is_valid
    
    # Test invalid message type (999 doesn't exist)
    try:
        msg = MessageProtocol.parse("999|Test||data=value")
        is_valid, error = validator.validate_incoming(msg, "CONNECTED")
        print(f"Invalid type: is_valid={is_valid}, error={error}")
        assert not is_valid
        assert validator.invalid_count == 1
    except ValueError:
        print("Invalid type rejected by parser (expected)")
    
    # Test threshold
    print(f"Invalid count: {validator.invalid_count}")
    validator.reset()
    print(f"After reset: {validator.invalid_count}")
    assert validator.invalid_count == 0
    
    print("✓ Validator tests passed!\n")


def test_round_trip():
    """Test building and parsing (round trip)"""
    print("=== Testing Round Trip ===")
    
    # Build a message
    built = MessageProtocol.build(
        ClientMessageType.PLAY_CARDS,
        player_id="",
        room_id="",
        cards="2H,3D,4S"
    )
    print(f"Built message: {repr(built)}")
    
    # Remove newline and parse it back
    parsed = MessageProtocol.parse(built.strip())
    print(f"Parsed back: {parsed}")
    
    assert parsed.type == ClientMessageType.PLAY_CARDS
    assert parsed.data['cards'] == "2H,3D,4S"
    
    print("✓ Round trip test passed!\n")


def main():
    """Run all tests"""
    print("=" * 60)
    print("MESSAGE LAYER TEST SUITE")
    print("=" * 60 + "\n")
    
    try:
        test_message_building()
        test_message_parsing()
        test_message_buffer()
        test_message_validator()
        test_round_trip()
        
        print("=" * 60)
        print("✓ ALL TESTS PASSED!")
        print("=" * 60)
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
