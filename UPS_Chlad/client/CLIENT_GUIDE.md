# 📘 Gamba Server - Complete Client Implementation Guide

**Version:** 1.0  
**Last Updated:** October 28, 2025  
**Server Version:** 1.0

---

## 📑 Table of Contents

1. [Overview & Connection Details](#overview--connection-details)
2. [Protocol Specification](#protocol-specification)
3. [Message Types Reference](#message-types-reference)
4. [Complete Game Flow](#complete-game-flow)
5. [Game State Message Details](#game-state-message-details)
6. [Card Format & Representation](#card-format--representation)
7. [Game Rules](#game-rules)
8. [Heartbeat & Keep-Alive](#heartbeat--keep-alive)
9. [Error Handling](#error-handling)
10. [Client State Machine](#client-state-machine)
11. [Security & Best Practices](#security--best-practices)
12. [Implementation Checklist](#implementation-checklist)
13. [Testing Checklist](#testing-checklist)
14. [Example Messages](#example-messages)
15. [Critical Implementation Notes](#critical-implementation-notes)

---

## 🎯 Overview & Connection Details

### What is Gamba?
Gamba is a multiplayer card game server implementing a "shedding" style card game where players race to empty their hands and reserve cards. The game supports 2 players per room with special cards (2, 7, 10) that modify gameplay.

### Connection Information
- **Protocol**: TCP/IP
- **Default Address**: `127.0.0.1`
- **Default Port**: `8080` (configurable by server)
- **Message Format**: Text-based, newline-delimited
- **Character Encoding**: UTF-8
- **Max Message Size**: 8KB
- **Connection Timeout**: 60 seconds without heartbeat

### Quick Start
```
1. Connect to 127.0.0.1:8080 via TCP
2. Send: 0|||name=YourName\n
3. Receive: 100|YourName||name=YourName|status=success\n
4. Send: 2|||\n (join room)
5. Wait for second player
6. Start sending PING every 30s: 4|||\n
7. When room full, send: 5|||\n (start game)
8. Play!
```

---

## 📡 Protocol Specification

### Message Format

**Structure:**
```
TYPE|PLAYER_ID|ROOM_ID|key1=value1|key2=value2|...\n
```

**Components:**

| Component | Required | Description | Example |
|-----------|----------|-------------|---------|
| `TYPE` | ✅ Yes | Integer message type (0-199) | `0` |
| `PLAYER_ID` | ⚠️ Optional | Your player name or target player | `Alice` |
| `ROOM_ID` | ⚠️ Optional | Room identifier | `ROOM_1` |
| `data` | ⚠️ Optional | Key-value pairs | `name=Alice` |
| `\n` | ✅ **CRITICAL** | Message terminator (newline) | Must end every message |

**Field Separators:**
- Between fields: `|` (pipe character)
- Between key-value pairs: `|` (pipe character)
- Key-value separator: `=` (equals sign)

**Empty Fields:**
Use empty string between pipes: `0|||name=Alice` (PLAYER_ID and ROOM_ID are empty)

### Message Parsing Example

```
Raw: "100|Alice||name=Alice|status=success\n"

Split by |:
  [0] = "100"           → TYPE = 100 (CONNECTED)
  [1] = "Alice"         → PLAYER_ID = "Alice"
  [2] = ""              → ROOM_ID = (empty)
  [3] = "name=Alice"    → data["name"] = "Alice"
  [4] = "status=success"→ data["status"] = "success"
```

### Message Construction Template

```java
String message = TYPE + "|" + PLAYER_ID + "|" + ROOM_ID + "|";
for (String key : data.keySet()) {
    message += key + "=" + data.get(key) + "|";
}
if (message.endsWith("|")) {
    message = message.substring(0, message.length() - 1);
}
message += "\n"; // CRITICAL: Add newline
```

---

## 📨 Message Types Reference

### Client → Server Messages

| Type | Name | Format | When to Send | Required Data |
|------|------|--------|--------------|---------------|
| `0` | CONNECT | `0\|\|\|name=NAME` | First message after TCP connect | `name` = player name (alphanumeric, _, -, max 32 chars) |
| `2` | JOIN_ROOM | `2\|\|\|` | After receiving CONNECTED | None |
| `4` | PING | `4\|\|\|` | Every 30 seconds | None |
| `5` | START_GAME | `5\|\|\|` | When room has 2 players | None |
| `6` | RECONNECT | `6\|\|\|name=NAME` | To reconnect after disconnect | `name` = your player name |
| `7` | PLAY_CARDS | `7\|\|\|cards=CARDS` | On your turn, to play cards | `cards` = comma-separated card list OR "RESERVE" |
| `8` | PICKUP_PILE | `8\|\|\|` | On your turn, when can't play | None |

### Server → Client Messages

| Type | Name | Key Fields | Description |
|------|------|------------|-------------|
| `100` | CONNECTED | `name`, `status` | Connection successful |
| `101` | ROOM_JOINED | `player_count`, `players`, `room_full` | Room joined or updated |
| `102` | ROOM_LEFT | `status` | You left the room |
| `103` | ERROR | `error`, `disconnect?` | Error message; may disconnect |
| `104` | PONG | None | Heartbeat response |
| `105` | GAME_STARTED | `status` | Game starting |
| `106` | GAME_STATE | See Game State section | Current game state |
| `107` | PLAYER_DISCONNECTED | `disconnected_player`, `status` | Opponent lost connection |
| `109` | PLAYER_RECONNECTED | `reconnected_player`, `status` | Opponent reconnected |
| `111` | TURN_RESULT | `result`, `status` | Your action result |
| `112` | GAME_OVER | `winner`, `reason`, `status` | Game finished |

---

## 🎮 Complete Game Flow

### Connection & Room Join

```
Client                          Server
  |-- TCP Connect --------------->|
  |-- 0|||name=Alice ------------>|
  |<-- 100|Alice||name=Alice ------|
  |-- 2||| ---------------------->|
  |<-- 101|Alice|ROOM_1|... ------|
```

### When Second Player Joins

```
You                             Server                          Other Player
  |                               |<-- 0|||name=Bob -------------|
  |                               |-- 100|Bob||... ------------->|
  |                               |<-- 2||| ---------------------|
  |<-- 101|Bob|ROOM_1|... --------|                              |
  |   (Bob joined broadcast)      |-- 101|Bob|ROOM_1|... ------->|
```

### Game Start

```
You                             Server                          Other Player
  |-- 5||| ---------------------->|                              |
  |<-- 105||ROOM_1|... -----------|                              |
  |                               |-- 105||ROOM_1|... ---------->|
  |<-- 106|Alice|ROOM_1|... ------|                              |
  |   (your game state)           |-- 106|Bob|ROOM_1|... ------->|
  |                               |   (Bob's game state)         |
```

### Playing Cards

```
You (Your Turn)                 Server                          Other Player
  |-- 7|||cards=2H,2D ----------->|                              |
  |<-- 111|Alice||... ------------|  (TURN_RESULT)               |
  |<-- 106|Alice|ROOM_1|... ------|  (Updated state)             |
  |                               |-- 106|Bob|ROOM_1|... ------->|
  |                               |   (Bob sees your play)       |
```

### Game Over

```
You                             Server                          Other Player
  |-- 7|||cards=AS ------------->|  (last card)                 |
  |<-- 111|Alice||... ------------|                              |
  |<-- 112|Alice|ROOM_1|... ------|  (GAME_OVER)                 |
  |                               |-- 112|Bob|ROOM_1|... ------->|
  |<-- 102|Alice||... ------------|  (ROOM_LEFT)                 |
  |                               |-- 102|Bob||... ------------->|
```

### Disconnection & Reconnection

**Short Disconnect (< 60s):**
```
You                             Server                          Other Player
  | [DISCONNECT]                  |                              |
  X                               |-- 107|Bob|ROOM_1|... ------->|
  |                               |   (You disconnected)         |
  | [RECONNECT]                   |                              |
  |-- 6|||name=Alice ------------>|                              |
  |<-- 100|Alice||... ------------|                              |
  |<-- 106|Alice|ROOM_1|... ------|  (Game state restored)       |
  |                               |-- 109|Bob|ROOM_1|... ------->|
  |                               |   (You reconnected)          |
```

**Long Disconnect (> 120s):**
```
You                             Server                          Other Player
  | [DISCONNECT]                  |                              |
  X                               | [60s timeout]                |
  |                               | [120s cleanup]               |
  |                               |-- 112|Bob|ROOM_1|... ------->|
  |                               |   (Bob wins, you timeout)    |
  |                               |-- 102|Bob||... ------------->|
```

---

## 🃏 Game State Message Details

### Message Format (Type 106)

```
106|PLAYER_NAME|ROOM_ID|field1=value1|field2=value2|...
```

### All Fields

| Field | Type | Description | Example |
|-------|------|-------------|---------|
| `hand` | string | Your cards (comma-separated) | `2H,AS,10D` |
| `reserves` | int | Your reserve card count | `3` |
| `current_player` | string | Whose turn it is | `Alice` |
| `top_card` | string | Top card on discard pile | `5H` |
| `your_turn` | bool | Is it your turn? | `true` or `false` |
| `must_play_low` | bool | Must play ≤7? | `false` |
| `deck_size` | int | Cards left in draw pile | `0` |
| `discard_pile_size` | int | Cards in discard pile | `5` |
| `opponent_hand` | int | Opponent's hand size | `4` |
| `opponent_reserves` | int | Opponent's reserve count | `3` |
| `opponent_name` | string | Opponent's name | `Bob` |

### Example Game States

**Game Start:**
```
106|Alice|ROOM_1|current_player=Alice|deck_size=0|discard_pile_size=0|hand=2H,AS,10D|must_play_low=false|opponent_hand=3|opponent_name=Bob|opponent_reserves=3|reserves=3|top_card=1S|your_turn=true
```

**After Playing Cards:**
```
106|Bob|ROOM_1|current_player=Bob|deck_size=0|discard_pile_size=2|hand=5H,6D,KC|must_play_low=false|opponent_hand=1|opponent_name=Alice|opponent_reserves=3|reserves=3|top_card=2H|your_turn=true
```

**After 7 Played (must play low):**
```
106|Alice|ROOM_1|current_player=Alice|deck_size=0|discard_pile_size=5|hand=2D,3H,KS|must_play_low=true|opponent_hand=2|opponent_name=Bob|opponent_reserves=3|reserves=3|top_card=7H|your_turn=true
```

**Playing Reserves (hand empty):**
```
106|Alice|ROOM_1|current_player=Alice|deck_size=0|discard_pile_size=8|hand=|must_play_low=false|opponent_hand=0|opponent_name=Bob|opponent_reserves=1|reserves=1|top_card=10D|your_turn=true
```

---

## 🎴 Card Format & Representation

### Card Code Format

**Structure:** `RANK + SUIT`

### Ranks

| Code | Name | Value | Special |
|------|------|-------|---------|
| `2` | Two | 2 | **Wild card** |
| `3-6` | Three-Six | 3-6 | Regular |
| `7` | Seven | 7 | **Forces ≤7** |
| `8-9` | Eight-Nine | 8-9 | Regular |
| `10` | Ten | 10 | **Burns pile** |
| `J` | Jack | 11 | Regular |
| `Q` | Queen | 12 | Regular |
| `K` | King | 13 | Regular |
| `A` | Ace | 14 | Highest |

### Suits

| Code | Name | Symbol |
|------|------|--------|
| `H` | Hearts | ♥ |
| `D` | Diamonds | ♦ |
| `C` | Clubs | ♣ |
| `S` | Spades | ♠ |

### Special Cards

**2 (Two) - Wild Card:**
- Can be played on any card
- Any card can be played on 2

**7 (Seven) - Low Enforcer:**
- Next player must play ≤7
- After next play, resets to normal

**10 (Ten) - Pile Burner:**
- Removes entire pile from game
- Next player starts fresh (any card)

**1S - Empty Pile Marker:**
- Not a real card
- Indicates pile is empty
- Any card can be played

### Card Parsing Example

```java
public class Card {
    String code;      // "2H"
    String rank;      // "2"
    String suit;      // "H"
    int value;        // 2
    
    public Card(String code) {
        this.code = code;
        this.rank = code.substring(0, code.length() - 1);
        this.suit = code.substring(code.length() - 1);
        this.value = getRankValue(rank);
    }
    
    private int getRankValue(String rank) {
        switch(rank) {
            case "A": return 14;
            case "K": return 13;
            case "Q": return 12;
            case "J": return 11;
            default: return Integer.parseInt(rank);
        }
    }
}
```

---

## 🎲 Game Rules

### Objective
Be the first to get rid of all cards (hand + reserves).

### Setup
- Each player: 3 hand cards + 3 reserve cards
- Draw pile: Empty (all dealt at start)
- Discard pile: Empty at start

### Turn Structure

**On your turn, you must:**
1. Play cards (if valid), OR
2. Play reserve (if hand empty), OR
3. Pick up pile (if cannot play)

### Playing Cards

**Basic Rule:** Play card ≥ top card OR special card (2, 7, 10)

**Multiple Cards:** Can play multiple of same rank (e.g., `2H,2D,2C`)

**Valid Play Examples:**
```
Top: 5H
Valid: 5D, 6H, AS, 2C, 7D, 10S
Invalid: 3H, 4D

Top: 7H (must_play_low=true)
Valid: 2D, 3H, 4C, 5S, 6D, 7S
Invalid: 8H, 9D, 10C, JH, QS, KD, AS

Top: 1S (empty)
Valid: ANY card
```

### Special Card Effects

**2 (Wild):**
- Play on anything
- Anything can play on it

**7 (Low Enforcer):**
- Next player must play ≤7
- Resets after next play

**10 (Burner):**
- Removes pile from game
- Next player starts fresh

### Reserve Cards

**When:** Hand empty AND deck empty

**How:** Send `7|||cards=RESERVE`

**Result:** Server reveals card and validates
- Valid → Card played
- Invalid → You pick up pile

### Picking Up Pile

**When:** No valid cards to play

**How:** Send `8|||`

**Result:** Pile goes to your hand

### Winning

**Condition:** hand=0 AND reserves=0

Server sends GAME_OVER to both players

---

## 💓 Heartbeat & Keep-Alive

### Critical for Connection

**Send every 30 seconds:**
```
4|||
```

**Server responds:**
```
104||
```

### Timeout Behavior

| Time | Status | Can Reconnect? |
|------|--------|----------------|
| 0-60s | Connected | Yes (auto) |
| 60s | Temporarily disconnected | Yes (manual RECONNECT) |
| 120s | Removed from server | No (use CONNECT for new session) |

### Implementation

```java
Timer pingTimer = new Timer();
pingTimer.schedule(new TimerTask() {
    @Override
    public void run() {
        sendMessage("4|||\n");
    }
}, 0, 30000); // Every 30 seconds
```

---

## ⚠️ Error Handling

### Error Message Format (Type 103)

```
103|||error=ERROR_MESSAGE
```

**With disconnect flag:**
```
103|||error=ERROR_MESSAGE|disconnect=true
```

### Common Errors

| Error | Cause | Solution |
|-------|-------|----------|
| `Player name cannot be empty` | Empty name in CONNECT | Provide valid name |
| `Connection failed - name already taken` | Name in use | Use different name or RECONNECT |
| `Must connect first` | Message before CONNECT | Send CONNECT first |
| `Not in any room` | Game action without room | Join room first |
| `Cannot start game` | < 2 players | Wait for second player |
| `Invalid card play` | Card doesn't follow rules | Check game rules |
| `Player name required` | Empty name in RECONNECT | Include name parameter |
| `Reconnection failed` | Player not found | Use CONNECT for new session |
| `Cannot reconnect as different player` | Security check | Disconnect first |

---

## 🔄 Client State Machine

```
DISCONNECTED
    ↓ [CONNECT]
CONNECTED (lobby)
    ↓ [JOIN_ROOM]
IN_ROOM (waiting)
    ↓ [START_GAME or other player starts]
PLAYING
    ↓ [GAME_OVER]
CONNECTED (auto back to lobby)
```

### State Transitions

| From | Event | To | Action |
|------|-------|-----|--------|
| DISCONNECTED | TCP connect | DISCONNECTED | Send CONNECT |
| DISCONNECTED | CONNECTED received | CONNECTED | Enable join button |
| CONNECTED | JOIN_ROOM sent | CONNECTED | Wait for ROOM_JOINED |
| CONNECTED | ROOM_JOINED received | IN_ROOM | Show room info |
| IN_ROOM | GAME_STARTED received | PLAYING | Show game board |
| PLAYING | GAME_OVER received | CONNECTED | Show result, back to lobby |
| ANY | Connection lost | DISCONNECTED | Try RECONNECT |

---

## 🔒 Security & Best Practices

### Name Validation
- Alphanumeric, underscore, hyphen only
- Max 32 characters
- No special characters

### Connection Security
- One account per connection
- Cannot reconnect as different player
- Use RECONNECT for existing session
- Use CONNECT for new session only

### Message Validation
- Always validate server responses
- Check `status` field
- Handle ERROR messages
- Verify `your_turn` before play

### Reconnection Strategy

```java
if (disconnected) {
    if (wasConnected && disconnectedLessThan2Minutes) {
        send("6|||name=" + myName);  // RECONNECT
    } else {
        send("0|||name=" + myName);  // New CONNECT
    }
}
```

### Resource Management
- Send PING every 30s
- Timeout detection: No PONG within 10s
- Buffer incomplete messages
- Handle partial receives

---

## ✅ Implementation Checklist

### Must-Have Features
- ✅ TCP socket connection
- ✅ Message parser (split by `|` and `=`)
- ✅ Message builder (construct with `\n`)
- ✅ PING timer (every 30s)
- ✅ CONNECT on startup
- ✅ JOIN_ROOM after connect
- ✅ Game state tracking
- ✅ UI updates based on GAME_STATE
- ✅ Card validation before sending
- ✅ Error handling (type 103)

### Recommended Features
- ✅ Auto-reconnection logic
- ✅ Connection status indicator
- ✅ Turn indicator (your turn highlight)
- ✅ Opponent status (disconnected/reconnected)
- ✅ Game log/history
- ✅ Network timeout detection

### Nice-to-Have
- 🎯 Card play animations
- 🎯 Sound effects
- 🎯 Valid move highlighting
- 🎯 Auto-play suggestion
- 🎯 Statistics tracking

---

## 🧪 Testing Checklist

### Basic Connection
- ✅ Connect to server
- ✅ Join room
- ✅ Second player joins
- ✅ Start game
- ✅ Receive initial game state

### Gameplay
- ✅ Play valid cards
- ✅ Server rejects invalid plays
- ✅ Pick up pile
- ✅ Play reserve cards
- ✅ Win condition (all cards gone)

### Disconnection
- ✅ Disconnect during game
- ✅ Other player notified
- ✅ Reconnect within 60s (short-term)
- ✅ Game state restored
- ✅ Disconnect for 2+ minutes (long-term)
- ✅ Game ends, winner declared

### Edge Cases
- ✅ Invalid message → disconnected
- ✅ Name already taken
- ✅ Play when not your turn
- ✅ Try to reconnect as different player
- ✅ Connection timeout (no ping)

---

## 📖 Example Messages

### Complete Session

```
→ 0|||name=Alice\n
← 100|Alice||name=Alice|status=success\n

→ 2|||\n
← 101|Alice|ROOM_1|player_count=1|players=Alice|room_full=false|status=success\n

← 101|Bob|ROOM_1|broadcast_type=room_notification|joined_player=Bob|player_count=2|players=Alice,Bob|room_full=true|status=success\n

→ 5|||\n
← 105||ROOM_1|status=started\n
← 106|Alice|ROOM_1|current_player=Alice|deck_size=0|discard_pile_size=0|hand=2H,AS,10D|must_play_low=false|opponent_hand=3|opponent_name=Bob|opponent_reserves=3|reserves=3|top_card=1S|your_turn=true\n

→ 4|||\n
← 104||\n

→ 7|||cards=2H\n
← 111|Alice||result=play_success|status=success\n
← 106|Alice|ROOM_1|current_player=Bob|deck_size=0|discard_pile_size=1|hand=AS,10D|must_play_low=false|opponent_hand=3|opponent_name=Bob|opponent_reserves=3|reserves=3|top_card=2H|your_turn=false\n

← 106|Alice|ROOM_1|current_player=Alice|deck_size=0|discard_pile_size=2|hand=AS,10D|must_play_low=false|opponent_hand=2|opponent_name=Bob|opponent_reserves=3|reserves=3|top_card=5H|your_turn=true\n

→ 7|||cards=AS\n
← 111|Alice||result=play_success|status=success\n
← 112|Bob|ROOM_1|winner=Bob|reason=no_cards_remaining|status=game_over\n
← 102|Alice||status=left\n
```

---

## 🚨 Critical Implementation Notes

1. **Always end messages with `\n`** - Server uses newline as delimiter
2. **Parse incrementally** - Messages may arrive in chunks
3. **Buffer incomplete messages** - Don't assume complete messages
4. **Send PING every 30s** - Critical for connection
5. **Check `your_turn` before play** - Prevent invalid actions
6. **Update UI on every GAME_STATE** - Even if not your turn
7. **Display opponent status** - Show disconnected/reconnected
8. **Validate locally first** - Check rules before sending
9. **Handle GAME_OVER** - Auto-return to lobby
10. **Log all messages** - Essential for debugging

### Message Buffering Example

```java
String buffer = "";

void onReceive(String data) {
    buffer += data;
    
    while (buffer.contains("\n")) {
        int pos = buffer.indexOf("\n");
        String message = buffer.substring(0, pos);
        buffer = buffer.substring(pos + 1);
        
        processMessage(message);
    }
}
```

### Turn Validation Example

```java
void onPlayButtonClick() {
    if (!gameState.get("your_turn").equals("true")) {
        showError("Not your turn!");
        return;
    }
    
    if (!isValidPlay(selectedCards)) {
        showError("Invalid play!");
        return;
    }
    
    String cards = String.join(",", selectedCards);
    sendMessage("7|||cards=" + cards + "\n");
}
```

---

## 🔧 Troubleshooting Guide

### Connection Issues

**Problem:** Cannot connect to server
- Check server is running
- Verify IP and port (default: 127.0.0.1:8080)
- Check firewall settings

**Problem:** Connection drops frequently
- Ensure PING sent every 30s
- Check network stability
- Verify message format (ends with `\n`)

### Message Issues

**Problem:** Server disconnects after message
- Check message format: `TYPE|||data\n`
- Verify newline terminator
- Ensure no special characters in name

**Problem:** Messages not parsed correctly
- Buffer incomplete messages
- Split by `\n` first
- Then split by `|`

### Game Issues

**Problem:** Cannot play cards
- Check `your_turn=true`
- Verify card validity against `top_card`
- Check `must_play_low` flag

**Problem:** Game state not updating
- Ensure processing all type 106 messages
- Update UI on every GAME_STATE
- Don't skip messages

### Reconnection Issues

**Problem:** RECONNECT fails
- Try within 120 seconds
- Use exact same name
- Disconnect first if connected as different player

**Problem:** Cannot connect after disconnect
- Wait 1 second after disconnect
- Use RECONNECT (type 6) not CONNECT (type 0)
- Check name matches original

---

## 📚 Additional Resources

- **Server runs on**: `127.0.0.1:8080` (default)
- **Protocol**: Text-based, newline-delimited
- **Max message size**: 8KB
- **Player timeout**: 60 seconds without ping
- **Reconnection window**: 120 seconds
- **Room capacity**: 2 players
- **Character encoding**: UTF-8

---

**Good luck with your client implementation! 🎮🃏**
