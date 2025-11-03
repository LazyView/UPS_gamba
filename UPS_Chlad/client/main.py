#!/usr/bin/env python3
"""
Gamba Card Game Client - Entry Point

Main entry point for the Gamba multiplayer card game client.
"""

import sys
import os

# Add src to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'src'))

from PyQt5.QtWidgets import QApplication
from gui import MainWindow


def main():
    """Main entry point"""
    # Create application
    app = QApplication(sys.argv)
    app.setApplicationName("Gamba Card Game")
    app.setOrganizationName("KIV/UPS")
    
    # Create and show main window
    window = MainWindow()
    window.show()
    
    # Run event loop
    return app.exec_()


if __name__ == "__main__":
    sys.exit(main())
