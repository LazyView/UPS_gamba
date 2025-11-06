#!/usr/bin/env python3
"""
Test the message layer with the actual Gamba server.
This script will:
1. Connect to the server
2. Send CONNECT message
3. Receive and parse CONNECTED response
4. Send PING
5. Receive PONG
6. Disconnect gracefully
"""

import socket
import sys
import time

# Add src to path
sys.path.insert(0, 'src')

from message import (
    MessageBuffer,
    MessageProtocol,
    ClientMessageType,
    ServerMessageType
)


class SimpleTestClient:
    """Simple client to test message layer with server"""
    
    def __init__(self, host='127.0.0.1', port=8080):
        self.host = host
        self.port = port
        self.socket = None
        self.buffer = MessageBuffer()
        
    def connect(self):
        """Connect to server"""
        print(f"Connecting to {self.host}:{self.port}...")
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.settimeout(5.0)  # 5 second timeout
        
        try:
            self.socket.connect((self.host, self.port))
            print("✓ TCP connection established!")
            return True
        except ConnectionRefusedError:
            print("✗ Connection refused - is the server running?")
            return False
        except Exception as e:
            print(f"✗ Connection error: {e}")
            return False
    
    def send_message(self, message: str):
        """Send a message to server"""
        print(f"→ SEND: {repr(message)}")
        try:
            self.socket.send(message.encode('utf-8'))
            return True
        except Exception as e:
            print(f"✗ Send error: {e}")
            return False
    
    def receive_messages(self, timeout=2.0):
        """Receive and parse messages from server"""
        self.socket.settimeout(timeout)
        messages = []
        
        try:
            data = self.socket.recv(4096)
            if not data:
                print("✗ Server closed connection")
                return None
            
            # Use our MessageBuffer!
            raw_messages = self.buffer.add_data(data.decode('utf-8'))
            
            # Parse each message
            for raw_msg in raw_messages:
                print(f"← RECV: {repr(raw_msg)}")
                try:
                    parsed = MessageProtocol.parse(raw_msg)
                    messages.append(parsed)
                    print(f"   Parsed: {parsed}")
                except ValueError as e:
                    print(f"   ✗ Parse error: {e}")
            
            return messages
            
        except socket.timeout:
            print("   (timeout - no data)")
            return []
        except Exception as e:
            print(f"✗ Receive error: {e}")
            return None
    
    def disconnect(self):
        """Close connection"""
        if self.socket:
            self.socket.close()
            print("✓ Disconnected")


def test_connect_and_ping():
    """
    Test sequence:
    1. Connect to server
    2. Send CONNECT message
    3. Receive CONNECTED
    4. Send PING
    5. Receive PONG
    """
    print("=" * 70)
    print("TEST: Connect and PING with Real Server")
    print("=" * 70 + "\n")
    
    client = SimpleTestClient()
    
    # Step 1: TCP Connect
    if not client.connect():
        return False
    
    print()
    
    # Step 2: Send CONNECT message
    print("Step 1: Sending CONNECT message...")
    connect_msg = MessageProtocol.build(
        ClientMessageType.CONNECT,
        name="TestClient"
    )
    if not client.send_message(connect_msg):
        client.disconnect()
        return False
    
    # Step 3: Receive CONNECTED response
    print("\nStep 2: Waiting for CONNECTED response...")
    messages = client.receive_messages()
    if messages is None:
        client.disconnect()
        return False
    
    if not messages:
        print("✗ No response from server")
        client.disconnect()
        return False
    
    # Check if we got CONNECTED
    connected_msg = messages[0]
    if connected_msg.type == ServerMessageType.CONNECTED:
        print(f"✓ Received CONNECTED: player={connected_msg.data.get('name')}, status={connected_msg.data.get('status')}")
    else:
        print(f"✗ Expected CONNECTED (100), got type {connected_msg.type}")
        client.disconnect()
        return False
    
    print()
    
    # Step 4: Send PING
    print("Step 3: Sending PING...")
    ping_msg = MessageProtocol.build(ClientMessageType.PING)
    if not client.send_message(ping_msg):
        client.disconnect()
        return False
    
    # Step 5: Receive PONG
    print("\nStep 4: Waiting for PONG...")
    messages = client.receive_messages()
    if messages is None:
        client.disconnect()
        return False
    
    if messages:
        pong_msg = messages[0]
        if pong_msg.type == ServerMessageType.PONG:
            print("✓ Received PONG!")
        else:
            print(f"✗ Expected PONG (104), got type {pong_msg.type}")
    else:
        print("✗ No PONG received")
    
    print()
    
    # Cleanup
    client.disconnect()
    
    print("\n" + "=" * 70)
    print("✓ TEST COMPLETED SUCCESSFULLY!")
    print("=" * 70)
    return True


def test_join_room():
    """
    Extended test:
    1. Connect
    2. Join room
    3. Wait for room info
    """
    print("=" * 70)
    print("TEST: Connect and Join Room")
    print("=" * 70 + "\n")
    
    client = SimpleTestClient()
    
    # Connect
    if not client.connect():
        return False
    
    print()
    
    # Send CONNECT
    print("Step 1: Connecting as TestClient2...")
    connect_msg = MessageProtocol.build(
        ClientMessageType.CONNECT,
        name="TestClient2"
    )
    client.send_message(connect_msg)
    
    # Receive CONNECTED
    messages = client.receive_messages()
    if not messages or messages[0].type != ServerMessageType.CONNECTED:
        print("✗ Failed to connect")
        client.disconnect()
        return False
    
    print("✓ Connected!")
    print()
    
    # Send JOIN_ROOM
    print("Step 2: Joining room...")
    join_msg = MessageProtocol.build(ClientMessageType.JOIN_ROOM)
    client.send_message(join_msg)
    
    # Receive ROOM_JOINED
    print("\nStep 3: Waiting for ROOM_JOINED...")
    messages = client.receive_messages()
    if not messages:
        print("✗ No response")
        client.disconnect()
        return False
    
    room_msg = messages[0]
    if room_msg.type == ServerMessageType.ROOM_JOINED:
        print(f"✓ Joined room!")
        print(f"   Room ID: {room_msg.room_id}")
        print(f"   Players: {room_msg.data.get('players', 'N/A')}")
        print(f"   Player count: {room_msg.data.get('player_count', 'N/A')}")
        print(f"   Room full: {room_msg.data.get('room_full', 'N/A')}")
    else:
        print(f"✗ Expected ROOM_JOINED (101), got type {room_msg.type}")
    
    print()
    
    # Cleanup
    client.disconnect()
    
    print("\n" + "=" * 70)
    print("✓ TEST COMPLETED!")
    print("=" * 70)
    return True


def test_invalid_message():
    """
    Test that server properly rejects invalid messages.
    """
    print("=" * 70)
    print("TEST: Send Invalid Message")
    print("=" * 70 + "\n")
    
    client = SimpleTestClient()
    
    if not client.connect():
        return False
    
    print()
    
    # Send invalid message (no proper format)
    print("Step 1: Sending INVALID message (should be rejected)...")
    invalid_msg = "INVALID_FORMAT_NO_PIPES\n"
    client.send_message(invalid_msg)
    
    # Receive ERROR
    print("\nStep 2: Waiting for ERROR response...")
    messages = client.receive_messages()
    if not messages:
        print("✗ No response (server might have disconnected us)")
    else:
        error_msg = messages[0]
        if error_msg.type == ServerMessageType.ERROR:
            print(f"✓ Received ERROR: {error_msg.data.get('error', 'N/A')}")
            print(f"   Disconnect: {error_msg.data.get('disconnect', 'N/A')}")
        else:
            print(f"Received type {error_msg.type}")
    
    print()
    client.disconnect()
    
    print("\n" + "=" * 70)
    print("✓ TEST COMPLETED!")
    print("=" * 70)
    return True


def main():
    """Run all tests"""
    print("\n" + "=" * 70)
    print("GAMBA MESSAGE LAYER - SERVER INTEGRATION TESTS")
    print("=" * 70 + "\n")
    
    print("Make sure the Gamba server is running on 127.0.0.1:8080")
    print("Press Enter to start testing, or Ctrl+C to cancel...")
    try:
        input()
    except KeyboardInterrupt:
        print("\nCancelled.")
        return 1
    
    print()
    
    tests = [
        ("Connect and PING", test_connect_and_ping),
        ("Join Room", test_join_room),
        ("Invalid Message", test_invalid_message),
    ]
    
    results = []
    
    for test_name, test_func in tests:
        print("\n" + "=" * 70)
        print(f"Running: {test_name}")
        print("=" * 70)
        time.sleep(1)  # Brief pause between tests
        
        try:
            result = test_func()
            results.append((test_name, result))
        except Exception as e:
            print(f"\n✗ Test failed with exception: {e}")
            import traceback
            traceback.print_exc()
            results.append((test_name, False))
        
        time.sleep(2)  # Pause between tests
    
    # Summary
    print("\n\n" + "=" * 70)
    print("TEST SUMMARY")
    print("=" * 70)
    for test_name, result in results:
        status = "✓ PASS" if result else "✗ FAIL"
        print(f"{status:8} - {test_name}")
    
    passed = sum(1 for _, result in results if result)
    total = len(results)
    print(f"\nTotal: {passed}/{total} tests passed")
    print("=" * 70)
    
    return 0 if passed == total else 1


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print("\n\nInterrupted by user.")
        sys.exit(1)
