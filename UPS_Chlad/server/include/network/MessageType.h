//
// Created by chlad on 9/15/2025.
//

#ifndef MESSAGETYPE_H
#define MESSAGETYPE_H
enum class MessageType {
    // gamba-client -> Server
    CONNECT = 0,	    // Client requesting connection
    DISCONNECT = 1,	    // Client requesting disconnection
    JOIN_ROOM = 2,	    // Client requesting room joining
    LEAVE_ROOM = 3,	    // Clint requesting room leaving
    PING = 4,		    // Client pinging server
    START_GAME = 5,	    // Client requesting game start
    RECONNECT = 6,      // Client requesting reconnetion
    PLAY_CARDS = 7,     // Client plays cards
    PICKUP_PILE = 8,    // Client picks up discard pile

    // Server -> gamba-client
    CONNECTED = 100, 			// Server notifies about connection
    ROOM_JOINED = 101,			// Sever notifies about Player joining room
    ROOM_LEFT = 102,			// Server notifies about Player leaving room
    ERROR_MSG = 103,			// Server notifies error message was sent
    PONG = 104,					// Server replies pong to client
    GAME_STARTED = 105,			// Server notifies game start
    GAME_STATE = 106,			// Server notifies game state
    PLAYER_DISCONNECTED = 107,  // Server notifies about player disconnection
    GAME_PAUSED = 108,          // Server notifies about game pause (not used in final implementation)
    PLAYER_RECONNECTED = 109,   // Server notifies about player reconnection
    GAME_RESUMED = 110,         // Server notifies about game resume (not used in final implementation)
    TURN_RESULT = 111,      // Server responds to turn actions
    GAME_OVER = 112         // Server announces game completion
};
#endif //MESSAGETYPE_H