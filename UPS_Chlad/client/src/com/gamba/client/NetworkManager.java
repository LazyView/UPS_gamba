// NetworkManager.java - Handle server communication
package com.gamba.client;

import java.io.*;
import java.net.*;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.atomic.AtomicBoolean;

public class NetworkManager {
    private final String host;
    private final int port;
    private Socket socket;
    private BufferedReader reader;
    private PrintWriter writer;

    private Thread readerThread;
    private Thread pingThread;
    private final AtomicBoolean connected = new AtomicBoolean(false);

    // Message handling
    private final BlockingQueue<ProtocolMessage> incomingMessages = new LinkedBlockingQueue<>();
    private MessageHandler messageHandler;

    // Player info
    private String playerId = "";
    private String playerName = "";
    private String currentRoomId = "";

    public NetworkManager(String host, int port) {
        this.host = host;
        this.port = port;
    }

    public boolean connect(String playerName) {
        try {
            this.playerName = playerName;

            // Create socket connection
            socket = new Socket();
            socket.connect(new InetSocketAddress(host, port), 5000); // 5 second timeout

            // Setup I/O streams
            reader = new BufferedReader(new InputStreamReader(socket.getInputStream()));
            writer = new PrintWriter(socket.getOutputStream(), true);

            connected.set(true);

            // Start reader thread
            readerThread = new Thread(this::readMessages);
            readerThread.setDaemon(true);
            readerThread.start();

            // Send connect message
            ProtocolMessage connectMsg = new ProtocolMessage(MessageType.CONNECT);
            connectMsg.setData("name", playerName);
            sendMessage(connectMsg);

            // Wait for connect response
            ProtocolMessage response = waitForMessage(MessageType.CONNECTED, 5000);
            if (response != null && "success".equals(response.getData("status"))) {
                this.playerId = response.getPlayerId();

                // Start ping thread
                pingThread = new Thread(this::sendPeriodicPings);
                pingThread.setDaemon(true);
                pingThread.start();

                System.out.println("Connected as " + playerName + " (ID: " + playerId + ")");
                return true;
            }

        } catch (Exception e) {
            System.err.println("Connection failed: " + e.getMessage());
            disconnect();
        }

        return false;
    }

    public void disconnect() {
        connected.set(false);

        try {
            if (writer != null) writer.close();
            if (reader != null) reader.close();
            if (socket != null) socket.close();
        } catch (Exception e) {
            System.err.println("Error during disconnect: " + e.getMessage());
        }

        if (readerThread != null) readerThread.interrupt();
        if (pingThread != null) pingThread.interrupt();
    }

    public boolean isConnected() {
        return connected.get() && socket != null && socket.isConnected();
    }

    public void sendMessage(ProtocolMessage message) {
        if (!isConnected()) {
            System.err.println("Cannot send message - not connected");
            return;
        }

        try {
            String serialized = message.serialize();
            System.out.println("SEND: " + serialized);
            writer.println(serialized);
            writer.flush();
        } catch (Exception e) {
            System.err.println("Failed to send message: " + e.getMessage());
            handleConnectionLost();
        }
    }

    private void readMessages() {
        try {
            String line;
            while (connected.get() && (line = reader.readLine()) != null) {
                try {
                    System.out.println("RECV: " + line);
                    ProtocolMessage message = ProtocolMessage.parse(line);

                    // Update local state based on message
                    updateLocalState(message);

                    // Add to queue for processing
                    incomingMessages.offer(message);

                    // Notify message handler if set
                    if (messageHandler != null) {
                        messageHandler.onMessageReceived(message);
                    }

                } catch (Exception e) {
                    System.err.println("Failed to parse message: " + line + " - " + e.getMessage());
                }
            }
        } catch (IOException e) {
            if (connected.get()) {
                System.err.println("Connection lost: " + e.getMessage());
                handleConnectionLost();
            }
        }
    }

    private void updateLocalState(ProtocolMessage message) {
        switch (message.getType()) {
            case ROOM_JOINED:
                if (message.getPlayerId().equals(playerId)) {
                    currentRoomId = message.getRoomId();
                }
                break;

            case ROOM_LEFT:
                if (message.getPlayerId().equals(playerId)) {
                    currentRoomId = "";
                }
                break;

            case GAME_OVER:
                // Game finished, will likely get ROOM_LEFT next
                break;
        }
    }

    private void sendPeriodicPings() {
        while (connected.get()) {
            try {
                Thread.sleep(20000); // Ping every 20 seconds

                if (connected.get()) {
                    ProtocolMessage ping = new ProtocolMessage(MessageType.PING);
                    ping.setPlayerId(playerId);
                    sendMessage(ping);
                }
            } catch (InterruptedException e) {
                break;
            }
        }
    }

    private void handleConnectionLost() {
        connected.set(false);
        if (messageHandler != null) {
            messageHandler.onConnectionLost();
        }
    }

    public ProtocolMessage waitForMessage(MessageType type, long timeoutMs) {
        long startTime = System.currentTimeMillis();

        while (System.currentTimeMillis() - startTime < timeoutMs) {
            ProtocolMessage message = incomingMessages.poll();
            if (message != null && message.getType() == type) {
                return message;
            }

            try {
                Thread.sleep(10);
            } catch (InterruptedException e) {
                break;
            }
        }

        return null;
    }

    public ProtocolMessage getNextMessage() {
        return incomingMessages.poll();
    }

    // Convenience methods for common operations
    public void joinRoom(String roomId) {
        ProtocolMessage msg = new ProtocolMessage(MessageType.JOIN_ROOM);
        msg.setPlayerId(playerId);
        if (roomId != null && !roomId.isEmpty()) {
            msg.setRoomId(roomId);
        }
        sendMessage(msg);
    }

    public void leaveRoom() {
        if (!currentRoomId.isEmpty()) {
            ProtocolMessage msg = new ProtocolMessage(MessageType.LEAVE_ROOM);
            msg.setPlayerId(playerId);
            msg.setRoomId(currentRoomId);
            sendMessage(msg);
        }
    }

    public void startGame() {
        if (!currentRoomId.isEmpty()) {
            ProtocolMessage msg = new ProtocolMessage(MessageType.START_GAME);
            msg.setPlayerId(playerId);
            msg.setRoomId(currentRoomId);
            sendMessage(msg);
        }
    }

    public void playCards(String... cards) {
        if (!currentRoomId.isEmpty()) {
            ProtocolMessage msg = new ProtocolMessage(MessageType.PLAY_CARDS);
            msg.setPlayerId(playerId);
            msg.setRoomId(currentRoomId);
            msg.setData("cards", String.join(",", cards));
            sendMessage(msg);
        }
    }

    public void pickupPile() {
        if (!currentRoomId.isEmpty()) {
            ProtocolMessage msg = new ProtocolMessage(MessageType.PICKUP_PILE);
            msg.setPlayerId(playerId);
            msg.setRoomId(currentRoomId);
            sendMessage(msg);
        }
    }

    public String getNetworkAddress() {
        return host + ":" + port;
    }
    public String getPlayerId() { return playerId; }
    public String getPlayerName() { return playerName; }
    public String getCurrentRoomId() { return currentRoomId; }

    // Message handler interface
    public interface MessageHandler {
        void onMessageReceived(ProtocolMessage message);
        void onConnectionLost();
    }

    public void setMessageHandler(MessageHandler handler) {
        this.messageHandler = handler;
    }
}