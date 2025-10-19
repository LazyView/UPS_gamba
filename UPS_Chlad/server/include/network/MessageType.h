//
// Created by chlad on 9/15/2025.
//

#ifndef MESSAGETYPE_H
#define MESSAGETYPE_H
enum class MessageType {
    // gamba-client -> Server
    CONNECT = 0,	// gamba-client requesting connection
    DISCONNECT = 1,	// gamba-client requesting disconnection
    JOIN_ROOM = 2,	// gamba-client requesting room joining
    LEAVE_ROOM = 3,	// Clint requesting room leaving
    PING = 4,		// gamba-client pinging server
    START_GAME = 5,	// gamba-client requesting game start
    RECONNECT = 6,
    PLAY_CARDS = 7,         // gamba-client plays cards
    PICKUP_PILE = 8,        // gamba-client picks up discard pile

    // Server -> gamba-client
    CONNECTED = 100, 			// Server notifies about connection
    ROOM_JOINED = 101,			// Sever notifies about Player joining room
    ROOM_LEFT = 102,			// Server notifies about Player leaving room
    ERROR_MSG = 103,			// Server notifies error message was sent
    PONG = 104,					// Server replies pong to client
    GAME_STARTED = 105,			// Server notifies game start
    GAME_STATE = 106,			// Server notifies game state
    PLAYER_DISCONNECTED = 107,
    GAME_PAUSED = 108,
    PLAYER_RECONNECTED = 109,
    GAME_RESUMED = 110,
    TURN_RESULT = 111,      // Server responds to turn actions
    GAME_OVER = 112         // Server announces game completion
};
#endif //MESSAGETYPE_H