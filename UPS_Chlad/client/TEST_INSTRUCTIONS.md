# Testing Message Layer with Server

## Prerequisites

1. **Start the Gamba Server** (in the server directory):
   ```bash
   cd ../server
   ./gamba_server
   ```
   
   Server should start on `127.0.0.1:8080` (check server.conf)

2. **Run the test client** (in this client directory):
   ```bash
   python test_with_server.py
   ```

## What the Test Does

The test script verifies that our message layer works correctly with the real server:

### Test 1: Connect and PING
- ✅ Establishes TCP connection
- ✅ Sends CONNECT message using `MessageProtocol.build()`
- ✅ Receives and parses CONNECTED response using `MessageProtocol.parse()`
- ✅ Sends PING, receives PONG
- ✅ Tests message buffering with `MessageBuffer`

### Test 2: Join Room
- ✅ Connects to server
- ✅ Joins a room using JOIN_ROOM message
- ✅ Receives ROOM_JOINED with room info
- ✅ Verifies data parsing (room_id, players, etc.)

### Test 3: Invalid Message
- ✅ Sends malformed message (tests server's error handling)
- ✅ Receives ERROR message
- ✅ Verifies disconnect flag

## Expected Output

```
==============================================================
GAMBA MESSAGE LAYER - SERVER INTEGRATION TESTS
==============================================================

Make sure the Gamba server is running on 127.0.0.1:8080
Press Enter to start testing, or Ctrl+C to cancel...

==============================================================
Running: Connect and PING
==============================================================

Connecting to 127.0.0.1:8080...
✓ TCP connection established!

Step 1: Sending CONNECT message...
→ SEND: '0|||name=TestClient\n'

Step 2: Waiting for CONNECTED response...
← RECV: '100|TestClient||name=TestClient|status=success'
   Parsed: ProtocolMessage(type=100, player=TestClient, room=, data={'name': 'TestClient', 'status': 'success'})
✓ Received CONNECTED: player=TestClient, status=success

Step 3: Sending PING...
→ SEND: '4|||\n'

Step 4: Waiting for PONG...
← RECV: '104||'
   Parsed: ProtocolMessage(type=104, player=, room=, data={})
✓ Received PONG!

✓ Disconnected

==============================================================
✓ TEST COMPLETED SUCCESSFULLY!
==============================================================

[... more tests ...]

==============================================================
TEST SUMMARY
==============================================================
✓ PASS   - Connect and PING
✓ PASS   - Join Room
✓ PASS   - Invalid Message

Total: 3/3 tests passed
==============================================================
```

## Troubleshooting

### Connection Refused
```
✗ Connection refused - is the server running?
```
**Solution:** Start the server first: `cd ../server && ./gamba_server`

### Parse Errors
If you see parse errors, check:
- Server is using the correct protocol format
- Message buffer is handling partial messages correctly

### Timeout
If tests hang:
- Check server logs: `tail -f ../server/logs/gamba_server.log`
- Verify server is responding to connections

## What This Validates

✅ **MessageProtocol.build()** - Creates valid protocol messages
✅ **MessageProtocol.parse()** - Correctly parses server responses
✅ **MessageBuffer** - Handles partial TCP data correctly
✅ **Message types** - Enums work correctly (CONNECT=0, CONNECTED=100, etc.)
✅ **Integration** - Our message layer works with the real C++ server!

## Next Steps

Once these tests pass, we know the message layer is solid and we can move on to:
- **utils/** layer (logger, constants, config)
- **game/** layer (card, player, rules, state)
- **network/** layer (NetworkClient with threading)
- **UI/** layer (PyQt5 interface)
