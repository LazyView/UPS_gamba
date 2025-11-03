#!/usr/bin/env python3
"""
Test script for GameLogWidget.
Shows the game log with different message types.
"""

import sys
sys.path.insert(0, 'src')

from PyQt5.QtWidgets import QApplication, QWidget, QVBoxLayout, QHBoxLayout, QPushButton
from PyQt5.QtCore import QTimer
from gui.widgets.game_log_widget import GameLogWidget


def main():
    app = QApplication(sys.argv)
    
    # Create window
    window = QWidget()
    window.setWindowTitle("Game Log Widget Test")
    window.resize(400, 400)
    
    layout = QVBoxLayout()
    
    # Create log widget
    log = GameLogWidget()
    layout.addWidget(log)
    
    # Buttons
    button_layout = QHBoxLayout()
    
    btn_normal = QPushButton("Add Normal")
    btn_normal.clicked.connect(lambda: log.add_message("Alice played 2♥"))
    button_layout.addWidget(btn_normal)
    
    btn_system = QPushButton("Add System")
    btn_system.clicked.connect(lambda: log.add_system_message("Game started"))
    button_layout.addWidget(btn_system)
    
    btn_error = QPushButton("Add Error")
    btn_error.clicked.connect(lambda: log.add_error("Invalid move!"))
    button_layout.addWidget(btn_error)
    
    btn_success = QPushButton("Add Success")
    btn_success.clicked.connect(lambda: log.add_success("You won!"))
    button_layout.addWidget(btn_success)
    
    btn_clear = QPushButton("Clear")
    btn_clear.clicked.connect(log.clear)
    button_layout.addWidget(btn_clear)
    
    layout.addLayout(button_layout)
    
    window.setLayout(layout)
    
    # Add some initial messages
    log.add_system_message("Welcome to Gamba!")
    log.add_system_message("Waiting for opponent...")
    log.add_success("Bob joined the game")
    log.add_system_message("Game started!")
    log.add_player_action("You", "played 2♥")
    log.add_player_action("Bob", "played 5♦")
    log.add_player_action("You", "picked up pile")
    
    window.show()
    
    return app.exec_()


if __name__ == "__main__":
    sys.exit(main())
