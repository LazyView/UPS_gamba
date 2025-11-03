from enum import IntEnum

class ClientMessageType(IntEnum):
    """Messages sent by client to server"""
    CONNECT = 0
    JOIN_ROOM = 2
    PING = 4
    START_GAME = 5
    RECONNECT = 6
    PLAY_CARDS = 7
    PICKUP_PILE = 8

class ServerMessageType(IntEnum):
    """Messages received from server"""
    CONNECTED = 100
    ROOM_JOINED = 101
    ROOM_LEFT = 102
    ERROR = 103
    PONG = 104
    GAME_STARTED = 105
    GAME_STATE = 106
    PLAYER_DISCONNECTED = 107
    PLAYER_RECONNECTED = 109
    TURN_RESULT = 111
    GAME_OVER = 112