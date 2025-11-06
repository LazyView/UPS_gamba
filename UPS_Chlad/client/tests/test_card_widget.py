#!/usr/bin/env python3
"""
Test script for CardWidget.
Shows various cards to test the visual display.
"""

import sys
sys.path.insert(0, 'src')

from PyQt5.QtWidgets import QApplication, QWidget, QHBoxLayout, QVBoxLayout, QLabel
from PyQt5.QtCore import Qt
from gui.widgets.card_widget import CardWidget, EmptyCardSlot
from game import Card


def main():
    app = QApplication(sys.argv)
    
    # Create window
    window = QWidget()
    window.setWindowTitle("Card Widget Test")
    window.resize(900, 400)
    
    layout = QVBoxLayout()
    
    # Title
    title = QLabel("Card Widget Test - Click cards to select")
    title.setAlignment(Qt.AlignCenter)
    title.setStyleSheet("font-size: 16px; font-weight: bold; margin: 10px;")
    layout.addWidget(title)
    
    # Regular cards
    regular_layout = QHBoxLayout()
    regular_label = QLabel("Regular Cards:")
    regular_label.setStyleSheet("font-weight: bold;")
    layout.addWidget(regular_label)
    
    regular_cards = [
        Card("2H"),  # Wild
        Card("5D"),  # Regular
        Card("7C"),  # Must play low
        Card("10S"), # Burn
        Card("AS"),  # Ace
        Card("KH"),  # King
    ]
    
    for card in regular_cards:
        card_widget = CardWidget(card)
        card_widget.clicked.connect(lambda c, w=card_widget: on_card_clicked(c, w))
        regular_layout.addWidget(card_widget)
    
    regular_layout.addStretch()
    layout.addLayout(regular_layout)
    
    # Empty slots
    empty_layout = QHBoxLayout()
    empty_label = QLabel("Empty Slots:")
    empty_label.setStyleSheet("font-weight: bold;")
    layout.addWidget(empty_label)
    
    empty_slot1 = EmptyCardSlot("Reserve\n3")
    empty_slot2 = EmptyCardSlot("Empty")
    empty_layout.addWidget(empty_slot1)
    empty_layout.addWidget(empty_slot2)
    empty_layout.addStretch()
    layout.addLayout(empty_layout)
    
    # Status
    status = QLabel("Click cards to toggle selection")
    status.setAlignment(Qt.AlignCenter)
    status.setStyleSheet("color: gray; font-style: italic; margin: 10px;")
    layout.addWidget(status)
    
    layout.addStretch()
    window.setLayout(layout)
    
    def on_card_clicked(card, widget):
        widget.toggle_selected()
        if widget.is_selected:
            print(f"✓ Selected: {card}")
        else:
            print(f"✗ Deselected: {card}")
    
    window.show()
    
    return app.exec_()


if __name__ == "__main__":
    sys.exit(main())
