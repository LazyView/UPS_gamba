#!/usr/bin/env python3
"""
Test script for GameWidget.
Shows the complete game interface with sample data.
"""

import sys
sys.path.insert(0, 'src')

from PyQt5.QtWidgets import QApplication
from PyQt5.QtCore import QTimer
from gui import GameWidget
from game import GameState, Card


def main():
    app = QApplication(sys.argv)
    
    # Create game state with sample data
    game_state = GameState()
    game_state.initialize_player("TestPlayer")
    
    # Set up player hand (this adds cards to player.hand list)
    game_state.player.set_hand_from_string("2H,5D,7C,AS,KH")
    game_state.player.reserves_count = 3
    
    print(f"DEBUG: Player hand has {len(game_state.player.hand)} cards")
    for card in game_state.player.hand:
        print(f"  - {card}")
    
    # Set game state
    game_state.top_card = Card("10S")
    game_state.must_play_low = False
    game_state.in_game = True
    
    # Simulate full game state update (as if from server)
    game_state.update_from_game_state_message({
        'player_id': 'TestPlayer',
        'hand': '2H,5D,7C,AS,KH',  # Your hand
        'reserves': '3',
        'current_player': 'TestPlayer',  # Your turn
        'top_card': '10S',
        'must_play_low': 'false',
        'opponent_hand': '4',
        'opponent_reserves': '2',
        'opponent_name': 'Bob',
        'deck_size': '0',
        'discard_pile_size': '5',
        'your_turn': 'true'
    })
    
    print(f"DEBUG: After update, player hand has {len(game_state.player.hand)} cards")
    
    # Create game widget
    game_widget = GameWidget(game_state)
    game_widget.resize(1000, 700)
    game_widget.setWindowTitle("Game Widget Test")
    
    # Connect signals to see what happens
    def on_play_cards(cards_str):
        print(f"✓ Play cards requested: {cards_str}")
        game_widget.add_log_message(f"Played: {cards_str}", "success")
    
    def on_pickup():
        print("✓ Pickup pile requested")
        game_widget.add_log_message("Picked up pile", "normal")
    
    def on_reserve():
        print("✓ Play reserve requested")
        game_widget.add_log_message("Played from reserve", "highlight")
    
    game_widget.play_cards_requested.connect(on_play_cards)
    game_widget.pickup_pile_requested.connect(on_pickup)
    game_widget.play_reserve_requested.connect(on_reserve)
    
    # Update UI to reflect game state
    game_widget.update_game_state()
    
    # Add some log messages
    game_widget.add_log_message("Game started", "system")
    game_widget.add_log_message("Bob joined", "success")
    game_widget.add_log_message("Your turn!", "highlight")
    
    game_widget.show()
    
    return app.exec_()


if __name__ == "__main__":
    sys.exit(main())
