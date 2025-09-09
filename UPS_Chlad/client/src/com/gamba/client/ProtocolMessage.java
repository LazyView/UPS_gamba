// ProtocolMessage.java - Protocol message handling
package com.gamba.client;

import java.util.HashMap;
import java.util.Map;

// Message types matching your C++ server exactly
enum MessageType {
    // Client -> Server
    CONNECT(0),
    DISCONNECT(1),
    JOIN_ROOM(2),
    LEAVE_ROOM(3),
    PING(4),
    START_GAME(5),
    RECONNECT(6),
    PLAY_CARDS(7),
    PICKUP_PILE(8),

    // Server -> Client
    CONNECTED(100),
    ROOM_JOINED(101),
    ROOM_LEFT(102),
    ERROR_MSG(103),
    PONG(104),
    GAME_STARTED(105),
    GAME_STATE(106),
    PLAYER_DISCONNECTED(107),
    GAME_PAUSED(108),
    PLAYER_RECONNECTED(109),
    GAME_RESUMED(110),
    TURN_RESULT(111),
    GAME_OVER(112);

    private final int code;

    MessageType(int code) {
        this.code = code;
    }

    public int getCode() {
        return code;
    }

    public static MessageType fromCode(int code) {
        for (MessageType type : values()) {
            if (type.code == code) {
                return type;
            }
        }
        throw new IllegalArgumentException("Unknown message type code: " + code);
    }
}

// Protocol message implementation
public class ProtocolMessage {
    private MessageType type;
    private String playerId = "";
    private String roomId = "";
    private Map<String, String> data = new HashMap<>();

    public ProtocolMessage(MessageType type) {
        this.type = type;
    }

    // Serialize message to protocol format: TYPE|PLAYER_ID|ROOM_ID|KEY1=VALUE1|KEY2=VALUE2
    public String serialize() {
        StringBuilder sb = new StringBuilder();
        sb.append(type.getCode()).append("|");
        sb.append(playerId).append("|");
        sb.append(roomId);

        for (Map.Entry<String, String> entry : data.entrySet()) {
            sb.append("|").append(entry.getKey()).append("=").append(entry.getValue());
        }

        return sb.toString();
    }

    // Parse message from protocol format
    public static ProtocolMessage parse(String message) {
        String[] parts = message.split("\\|", -1); // -1 to keep empty strings

        if (parts.length < 3) {
            throw new IllegalArgumentException("Invalid message format: " + message);
        }

        // Parse message type
        int typeCode = Integer.parseInt(parts[0]);
        MessageType type = MessageType.fromCode(typeCode);

        ProtocolMessage msg = new ProtocolMessage(type);
        msg.playerId = parts[1];
        msg.roomId = parts[2];

        // Parse key-value pairs
        for (int i = 3; i < parts.length; i++) {
            String part = parts[i];
            int equalPos = part.indexOf('=');
            if (equalPos > 0 && equalPos < part.length() - 1) {
                String key = part.substring(0, equalPos);
                String value = part.substring(equalPos + 1);
                msg.data.put(key, value);
            }
        }

        return msg;
    }

    // Getters and setters
    public MessageType getType() { return type; }
    public void setType(MessageType type) { this.type = type; }

    public String getPlayerId() { return playerId; }
    public void setPlayerId(String playerId) { this.playerId = playerId != null ? playerId : ""; }

    public String getRoomId() { return roomId; }
    public void setRoomId(String roomId) { this.roomId = roomId != null ? roomId : ""; }

    public String getData(String key) {
        return data.get(key);
    }

    public String getData(String key, String defaultValue) {
        return data.getOrDefault(key, defaultValue);
    }

    public void setData(String key, String value) {
        data.put(key, value);
    }

    public boolean hasData(String key) {
        return data.containsKey(key);
    }

    public Map<String, String> getAllData() {
        return new HashMap<>(data);
    }

    @Override
    public String toString() {
        return String.format("ProtocolMessage{type=%s, playerId='%s', roomId='%s', data=%s}",
                type, playerId, roomId, data);
    }
}