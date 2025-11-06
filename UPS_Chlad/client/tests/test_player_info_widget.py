#!/usr/bin/env python3
"""
Test script for PlayerInfoWidget.
Shows the opponent info panel in different states.
"""

import sys
sys.path.insert(0, 'src')

from PyQt5.QtWidgets import QApplication, QWidget, QVBoxLayout, QHBoxLayout, QPushButton, QLabel
from PyQt5.QtCore import Qt, QTimer
from gui.widgets.player_info_widget import PlayerInfoWidget


def main():
    app = QApplication(sys.argv)
    
    # Create window
    window = QWidget()
    window.setWindowTitle("Player Info Widget Test")
    window.resize(600, 400)
    
    layout = QVBoxLayout()
    
    # Title
    title = QLabel("Player Info Widget Test")
    title.setAlignment(Qt.AlignCenter)
    title.setStyleSheet("font-size: 16px; font-weight: bold; margin: 10px;")
    layout.addWidget(title)
    
    # Create two player info widgets to compare
    widgets_layout = QHBoxLayout()
    
    # Widget 1 - Normal state
    widget1 = PlayerInfoWidget()
    widget1.update_info("Alice", 5, 3)
    widget1.set_current_turn(False)
    widgets_layout.addWidget(widget1)
    
    # Widget 2 - On turn
    widget2 = PlayerInfoWidget()
    widget2.update_info("Bob", 2, 1)
    widget2.set_current_turn(True)
    widgets_layout.addWidget(widget2)
    
    # Widget 3 - Disconnected
    widget3 = PlayerInfoWidget()
    widget3.update_info("Charlie", 7, 3)
    widget3.set_connected(False)
    widgets_layout.addWidget(widget3)
    
    widgets_layout.addStretch()
    layout.addLayout(widgets_layout)
    
    # Buttons to test state changes
    button_layout = QHBoxLayout()
    
    btn_decrease = QPushButton("Decrease Cards")
    btn_decrease.clicked.connect(lambda: decrease_cards(widget1))
    button_layout.addWidget(btn_decrease)
    
    btn_toggle_turn = QPushButton("Toggle Turn (Bob)")
    btn_toggle_turn.clicked.connect(lambda: widget2.set_current_turn(not widget2.is_current_turn))
    button_layout.addWidget(btn_toggle_turn)
    
    btn_toggle_connection = QPushButton("Toggle Connection (Charlie)")
    btn_toggle_connection.clicked.connect(lambda: widget3.set_connected(not widget3.is_connected))
    button_layout.addWidget(btn_toggle_connection)
    
    layout.addLayout(button_layout)
    
    # Status
    status = QLabel("Test different states using the buttons above")
    status.setAlignment(Qt.AlignCenter)
    status.setStyleSheet("color: gray; font-style: italic; margin: 10px;")
    layout.addWidget(status)
    
    layout.addStretch()
    window.setLayout(layout)
    
    def decrease_cards(widget):
        """Simulate card play"""
        if widget.hand_count > 0:
            widget.update_info(widget.player_name, widget.hand_count - 1, widget.reserves_count)
        elif widget.reserves_count > 0:
            widget.update_info(widget.player_name, 0, widget.reserves_count - 1)
    
    window.show()
    
    return app.exec_()


if __name__ == "__main__":
    sys.exit(main())
