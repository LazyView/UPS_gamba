#!/usr/bin/env python3
"""
Test script for the game layer.
Tests Card, Player, GameRules, and GameState.
"""

import sys
sys.path.insert(0, 'src')

from game import Card, Player, OpponentPlayer, GameRules, GameState, parse_cards


def test_card():
    """Test Card class"""
    print("=" * 70)
    print("TEST: Card")
    print("=" * 70)
    
    # Test card creation
    card = Card("2H")
    print(f"Created card: {card} (code={card.code}, rank={card.rank}, suit={card.suit}, value={card.value})")
    assert card.code == "2H"
    assert card.rank == "2"
    assert card.suit == "H"
    assert card.value == 2
    print("✓ Card creation works")
    
    # Test special cards
    two = Card("2D")
    seven = Card("7H")
    ten = Card("10S")
    
    assert two.is_wild()
    assert two.is_special()
    print("✓ 2 is wild card")
    
    assert seven.is_seven()
    assert seven.is_special()
    print("✓ 7 is special card")
    
    assert ten.is_ten()
    assert ten.is_special()
    print("✓ 10 is special card")
    
    # Test card comparison
    ace = Card("AS")
    king = Card("KH")
    five = Card("5D")
    
    assert ace.value > king.value
    assert king.value > five.value
    print("✓ Card value comparison works")
    
    # Test can_play_on logic
    # Normal play: same or higher
    assert king.can_play_on(five, must_play_low=False)  # K > 5
    assert not five.can_play_on(king, must_play_low=False)  # 5 < K
    print("✓ Normal play rules work")
    
    # Wild card can play on anything
    assert two.can_play_on(ace, must_play_low=False)
    assert two.can_play_on(king, must_play_low=False)
    print("✓ Wild card (2) can play on anything")
    
    # Must play low (≤7)
    assert five.can_play_on(seven, must_play_low=True)  # 5 ≤ 7
    assert not king.can_play_on(seven, must_play_low=True)  # K > 7
    assert two.can_play_on(seven, must_play_low=True)  # 2 is wild
    print("✓ Must play low (≤7) rules work")
    
    # Test parsing
    cards = parse_cards("2H,AS,10D")
    assert len(cards) == 3
    assert cards[0].code == "2H"
    assert cards[1].code == "AS"
    assert cards[2].code == "10D"
    print("✓ Card parsing works")
    
    # Test empty pile marker
    empty = Card("1S")
    assert empty.is_empty_marker()
    assert king.can_play_on(empty)  # Any card on empty
    print("✓ Empty pile marker works")
    
    print("✓ Card tests completed")
    print()


def test_player():
    """Test Player class"""
    print("=" * 70)
    print("TEST: Player")
    print("=" * 70)
    
    # Create player
    player = Player("Alice")
    print(f"Created player: {player}")
    assert player.name == "Alice"
    assert len(player.hand) == 0
    assert player.reserves_count == 0
    print("✓ Player creation works")
    
    # Test hand management
    player.set_hand_from_string("2H,AS,10D")
    assert len(player.hand) == 3
    assert player.has_card(Card("2H"))
    assert player.has_card(Card("AS"))
    print("✓ Hand management works")
    
    # Test card operations
    card_to_play = Card("2H")
    assert player.has_card(card_to_play)
    player.remove_cards_from_hand([card_to_play])
    assert len(player.hand) == 2
    assert not player.has_card(card_to_play)
    print("✓ Card removal works")
    
    # Test playable cards
    player.set_hand_from_string("2H,5D,KS")
    
    # Normal play (top card is 6H)
    top_card_normal = Card("6H")
    playable = player.get_playable_cards(top_card_normal, must_play_low=False)
    assert len(playable) == 2  # 2H (wild) and KS (≥6); 5D is too low (5 < 6)
    print("✓ Get playable cards works (normal)")
    
    # Must play low (7 was just played)
    top_card_seven = Card("7H")
    playable_low = player.get_playable_cards(top_card_seven, must_play_low=True)
    assert len(playable_low) == 2  # Only 2H (wild) and 5D (≤7); KS is too high
    print("✓ Get playable cards works (must_play_low)")
    
    # Test win condition
    player.clear_hand()
    player.reserves_count = 0
    assert player.has_won()
    print("✓ Win condition works")
    
    print("✓ Player tests completed")
    print()


def test_opponent_player():
    """Test OpponentPlayer class"""
    print("=" * 70)
    print("TEST: OpponentPlayer")
    print("=" * 70)
    
    opponent = OpponentPlayer("Bob")
    print(f"Created opponent: {opponent}")
    assert opponent.name == "Bob"
    assert opponent.hand_size == 0
    print("✓ OpponentPlayer creation works")
    
    # Simulate game state update
    game_state_data = {
        'opponent_hand': '3',
        'opponent_reserves': '2',
        'current_player': 'Bob',
        'opponent_name': 'Bob'
    }
    opponent.update_from_game_state(game_state_data)
    
    assert opponent.hand_size == 3
    assert opponent.reserves_count == 2
    assert opponent.is_current_player
    print("✓ OpponentPlayer update from game state works")
    
    print("✓ OpponentPlayer tests completed")
    print()


def test_game_rules():
    """Test GameRules class"""
    print("=" * 70)
    print("TEST: GameRules")
    print("=" * 70)
    
    player = Player("Alice")
    player.set_hand_from_string("2H,5D,AS,KS")
    
    # Test valid play
    cards_to_play = [Card("AS")]
    top_card = Card("7H")
    
    is_valid, error = GameRules.validate_card_play(player, cards_to_play, top_card, must_play_low=False)
    assert is_valid, f"Should be valid: {error}"
    print("✓ Valid play (A on 7) passes validation")
    
    # Test invalid play (must play low)
    is_valid, error = GameRules.validate_card_play(player, [Card("KS")], top_card, must_play_low=True)
    assert not is_valid, "Should be invalid (K > 7)"
    print(f"✓ Invalid play rejected: {error}")
    
    # Test wild card always valid
    is_valid, error = GameRules.validate_card_play(player, [Card("2H")], Card("AS"), must_play_low=True)
    assert is_valid, "Wild card should always be valid"
    print("✓ Wild card (2) is always valid")
    
    # Test multiple cards (same rank)
    player.set_hand_from_string("2H,2D,2C")
    is_valid, error = GameRules.validate_card_play(player, parse_cards("2H,2D"), Card("5S"), False)
    assert is_valid, f"Should allow multiple cards of same rank: {error}"
    print("✓ Multiple cards of same rank allowed")
    
    # Test multiple cards (different ranks - should fail)
    player.set_hand_from_string("2H,3D")
    is_valid, error = GameRules.validate_card_play(player, parse_cards("2H,3D"), Card("5S"), False)
    assert not is_valid, "Should not allow different ranks"
    print(f"✓ Different ranks rejected: {error}")
    
    # Test reserve play validation
    player.clear_hand()
    player.reserves_count = 3
    is_valid, error = GameRules.validate_reserve_play(player, Card("5H"), False)
    assert is_valid, f"Should allow reserve play: {error}"
    print("✓ Reserve play validation works")
    
    # Test pickup pile validation
    player.set_hand_from_string("3H,4D")
    top_card = Card("AS")  # Can't play on Ace
    is_valid, error = GameRules.validate_pickup_pile(player, top_card, False)
    assert is_valid, f"Should allow pickup: {error}"
    print("✓ Pickup pile validation works")
    
    # Test suggested play
    player.set_hand_from_string("2H,5D,AS,KS")
    
    # Normal play scenario (top card is 6H)
    suggested = GameRules.get_suggested_play(player, Card("6H"), False)
    assert suggested is not None
    assert suggested[0].value >= 6 or suggested[0].is_wild()  # Should suggest valid card (KS or AS or 2H)
    print(f"✓ Suggested play (normal): {[str(c) for c in suggested]}")
    
    # Must play low scenario (7 was just played)
    suggested_low = GameRules.get_suggested_play(player, Card("7H"), True)
    assert suggested_low is not None
    assert suggested_low[0].value <= 7 or suggested_low[0].is_wild()  # Should suggest ≤7 (2H or 5D)
    print(f"✓ Suggested play (must_play_low): {[str(c) for c in suggested_low]}")
    
    print("✓ GameRules tests completed")
    print()


def test_game_state():
    """Test GameState class"""
    print("=" * 70)
    print("TEST: GameState")
    print("=" * 70)
    
    # Create game state
    state = GameState()
    state.initialize_player("Alice")
    
    assert state.player_name == "Alice"
    assert state.player is not None
    print("✓ GameState initialization works")
    
    # Simulate game state update from server
    game_state_data = {
        'player_id': 'Alice',
        'hand': '2H,AS,10D',
        'reserves': '3',
        'current_player': 'Alice',
        'top_card': '5H',
        'must_play_low': 'false',
        'opponent_hand': '4',
        'opponent_reserves': '2',
        'opponent_name': 'Bob',
        'deck_size': '0',
        'discard_pile_size': '5'
    }
    
    state.update_from_game_state_message(game_state_data)
    
    assert len(state.player.hand) == 3
    assert state.player.reserves_count == 3
    assert state.your_turn()
    assert state.top_card.code == "5H"
    assert not state.must_play_low
    assert state.opponent is not None
    assert state.opponent.name == "Bob"
    assert state.opponent.hand_size == 4
    print("✓ Game state update from message works")
    
    # Test serialization
    serialized = state.serialize()
    print("✓ Serialization works")
    
    # Test restoration
    new_state = GameState()
    new_state.restore(serialized)
    
    assert new_state.player_name == "Alice"
    assert new_state.player is not None
    assert len(new_state.player.hand) == 3
    assert new_state.opponent is not None
    assert new_state.opponent.name == "Bob"
    print("✓ Restoration works")
    
    # Test game info
    info = state.get_game_info()
    print(f"Game info: {info}")
    assert "Alice" in info or "You" in info
    print("✓ Game info generation works")
    
    # Test reset
    state.reset_game()
    assert not state.in_game
    assert state.opponent is None
    print("✓ Game reset works")
    
    print("✓ GameState tests completed")
    print()


def main():
    """Run all game tests"""
    print("\n" + "=" * 70)
    print("GAME LAYER TEST SUITE")
    print("=" * 70 + "\n")
    
    try:
        test_card()
        test_player()
        test_opponent_player()
        test_game_rules()
        test_game_state()
        
        print("=" * 70)
        print("✓ ALL GAME TESTS PASSED!")
        print("=" * 70)
        return 0
        
    except AssertionError as e:
        print(f"\n✗ TEST FAILED: {e}")
        import traceback
        traceback.print_exc()
        return 1
    except Exception as e:
        print(f"\n✗ UNEXPECTED ERROR: {e}")
        import traceback
        traceback.print_exc()
        return 1


if __name__ == "__main__":
    sys.exit(main())
