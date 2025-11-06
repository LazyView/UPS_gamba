#!/usr/bin/env python3
"""
Test/demo script for MainWindow.
Shows the complete application flow.
"""

import sys
sys.path.insert(0, 'src')

from PyQt5.QtWidgets import QApplication
from gui import MainWindow


def main():
    """Run the application"""
    print("=" * 60)
    print("Gamba Client - Main Window Test")
    print("=" * 60)
    print("\nStarting application...")
    print("Connection dialog should appear automatically.")
    print("\nTo test:")
    print("  1. Enter connection details")
    print("  2. Click Connect")
    print("  3. Server must be running on 127.0.0.1:8080")
    print("  4. Watch state transitions")
    print("\n" + "=" * 60 + "\n")
    
    app = QApplication(sys.argv)
    
    # Create main window
    window = MainWindow()
    window.show()
    
    # Run application
    return app.exec_()


if __name__ == "__main__":
    sys.exit(main())
