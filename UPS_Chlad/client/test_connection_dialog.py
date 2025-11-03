#!/usr/bin/env python3
"""
Test script for ConnectionDialog.
Shows the dialog and prints the values when user connects.
"""

import sys
sys.path.insert(0, 'src')

from PyQt5.QtWidgets import QApplication
from gui import ConnectionDialog


def main():
    app = QApplication(sys.argv)
    
    # Create dialog
    dialog = ConnectionDialog()
    
    # Connect signal
    def on_connect_requested(host, port, name):
        print(f"\n{'='*50}")
        print("Connect Requested!")
        print(f"{'='*50}")
        print(f"Host: {host}")
        print(f"Port: {port}")
        print(f"Name: {name}")
        print(f"{'='*50}\n")
        
        # Simulate connection success
        from PyQt5.QtCore import QTimer
        QTimer.singleShot(1000, dialog.show_success)
    
    dialog.connect_requested.connect(on_connect_requested)
    
    # Show dialog
    result = dialog.exec_()
    
    if result == ConnectionDialog.Accepted:
        print("✓ Dialog accepted (user connected)")
        host, port, name = dialog.get_values()
        print(f"  Final values: {host}:{port} as '{name}'")
    else:
        print("✗ Dialog rejected (user cancelled)")
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
