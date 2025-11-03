#!/usr/bin/env python3
"""
Test reconnection with server kill/restart.
"""

import sys
sys.path.insert(0, 'src')

from PyQt5.QtWidgets import QApplication
from PyQt5.QtCore import QTimer
from network import ConnectionManager

app = QApplication(sys.argv)
manager = ConnectionManager()

# Track events
events = []

def on_state_changed(old, new):
    events.append(f"State: {old} → {new}")
    print(f"[EVENT] State: {old} → {new}")

def on_reconnecting():
    events.append("Reconnecting...")
    print("[EVENT] 🔄 Auto-reconnection started!")

def on_reconnected():
    events.append("Reconnected!")
    print("[EVENT] ✅ Reconnection successful!")

def on_error(msg):
    events.append(f"Error: {msg}")
    print(f"[EVENT] ❌ Error: {msg}")

def on_message(msg):
    print(f"[MSG] Type {msg.get('type')}: {msg.get('data')}")

manager.state_changed.connect(on_state_changed)
manager.reconnecting.connect(on_reconnecting)
manager.reconnected.connect(on_reconnected)
manager.error_occurred.connect(on_error)
manager.message_received.connect(on_message)

# Connect
print("\n🔌 Connecting...")
manager.connect("127.0.0.1", 8080, "ReconnectTest")

# Join room after 2 seconds
QTimer.singleShot(2000, manager.send_join_room)

# Instructions
def show_instructions():
    print("\n" + "="*60)
    print("TEST: Reconnection")
    print("="*60)
    print("Instructions:")
    print("1. Wait for connection and room join (should happen now)")
    print("2. Go to server terminal and press Ctrl+C to kill it")
    print("3. Watch client attempt auto-reconnect (every 2 seconds)")
    print("4. Restart server: ./gamba_server")
    print("5. Watch client reconnect successfully!")
    print("6. Press Ctrl+C here to exit")
    print("="*60 + "\n")

QTimer.singleShot(3000, show_instructions)

# Run
try:
    app.exec_()
except KeyboardInterrupt:
    print("\n\nTest ended by user.")
finally:
    print("\n" + "="*60)
    print("Event Summary:")
    for event in events:
        print(f"  - {event}")
    print("="*60)
    manager.disconnect()