# Testing Guide

## Overview

This directory contains test scripts for each layer of the Gamba client implementation.

## Test Scripts

### 1. `test_message_layer.py`
Tests the protocol message handling:
- Message building (creating protocol messages)
- Message parsing (parsing server responses)
- Message buffering (handling partial TCP data)
- Message validation

**Run:**
```bash
python test_message_layer.py
```

### 2. `test_utils_layer.py`
Tests utility modules:
- Logger (file and console logging)
- Config (settings save/load)
- Constants (all constants accessible)

**Run:**
```bash
python test_utils_layer.py
```

**Note:** Creates `logs/gamba_client.log` and temporary config file.

### 3. `test_game_layer.py`
Tests game logic:
- Card (parsing, special cards, can_play_on logic)
- Player (hand management, win conditions)
- GameRules (move validation, suggestions)
- GameState (state tracking, serialization)

**Run:**
```bash
python test_game_layer.py
```

### 4. `test_with_server.py`
Integration test with real server:
- Connect and PING
- Join room
- Invalid message handling

**Requirements:** Server must be running on 127.0.0.1:8080

**Run:**
```bash
python test_with_server.py
```

### 5. `run_all_tests.py`
Master test runner - runs all tests and provides summary.

**Run:**
```bash
python run_all_tests.py
```

## Expected Output

All tests should pass with output like:
```
============================================================
✓ ALL TESTS PASSED!
============================================================
```

If any test fails, you'll see detailed error messages.

## Test Coverage

✅ **Message Layer** - Protocol handling (TESTED with server)
✅ **Utils Layer** - Logging, config, constants
✅ **Game Layer** - Cards, players, rules, state
⏳ **Network Layer** - Not yet implemented
⏳ **UI Layer** - Not yet implemented

## Troubleshooting

### Import Errors
If you get import errors, make sure you're running from the `client/` directory:
```bash
cd client
python test_game_layer.py
```

### Logger Warnings
The logger creates a `logs/` directory automatically. This is expected behavior.

### Config File
`test_utils_layer.py` creates and cleans up `test_config.conf`. If you see this file, the test didn't complete properly.

## Next Steps

Once all tests pass:
1. ✅ Message, Utils, and Game layers are solid
2. Move on to Network layer (threading, connection management)
3. Then UI layer (PyQt5 interface)
4. Finally integration and testing with server
