// LobbyPanel.java - Room selection and management interface
package com.gamba.client;

import javax.swing.*;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.ArrayList;
import java.util.List;

public class LobbyPanel extends JPanel {
    private final NetworkManager networkManager;

    // GUI Components
    private JTextField roomIdField;
    private JButton joinRoomButton;
    private JButton createRoomButton;
    private JButton startGameButton;
    private JButton leaveRoomButton;

    // Room status
    private JLabel roomStatusLabel;
    private JTextArea playersArea;
    private JScrollPane playersScrollPane;

    // Current room info
    private String currentRoomId = "";
    private List<String> roomPlayers = new ArrayList<>();

    public LobbyPanel(NetworkManager networkManager) {
        this.networkManager = networkManager;
        initComponents();
    }

    private void initComponents() {
        setLayout(new BorderLayout());
        setBorder(BorderFactory.createTitledBorder("Game Lobby"));

        // Create room selection panel
        JPanel roomSelectionPanel = createRoomSelectionPanel();
        add(roomSelectionPanel, BorderLayout.NORTH);

        // Create room status panel
        JPanel roomStatusPanel = createRoomStatusPanel();
        add(roomStatusPanel, BorderLayout.CENTER);

        // Create control panel
        JPanel controlPanel = createControlPanel();
        add(controlPanel, BorderLayout.SOUTH);

        // Initial state
        updateRoomState();
    }

    private JPanel createRoomSelectionPanel() {
        JPanel panel = new JPanel(new GridBagLayout());
        panel.setBorder(BorderFactory.createTitledBorder("Room Selection"));

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.insets = new Insets(5, 5, 5, 5);

        // Room ID input
        gbc.gridx = 0; gbc.gridy = 0; gbc.anchor = GridBagConstraints.WEST;
        panel.add(new JLabel("Room ID (optional):"), gbc);

        gbc.gridx = 1; gbc.fill = GridBagConstraints.HORIZONTAL; gbc.weightx = 1.0;
        roomIdField = new JTextField("ROOM_1", 15);
        panel.add(roomIdField, gbc);

        // Buttons
        gbc.gridx = 2; gbc.fill = GridBagConstraints.NONE; gbc.weightx = 0;
        joinRoomButton = new JButton("Join Room");
        joinRoomButton.addActionListener(this::onJoinRoom);
        panel.add(joinRoomButton, gbc);

        gbc.gridx = 3;
        createRoomButton = new JButton("Create New Room");
        createRoomButton.addActionListener(this::onCreateRoom);
        panel.add(createRoomButton, gbc);

        return panel;
    }

    private JPanel createRoomStatusPanel() {
        JPanel panel = new JPanel(new BorderLayout());
        panel.setBorder(BorderFactory.createTitledBorder("Current Room"));

        // Room status label
        roomStatusLabel = new JLabel("Not in any room");
        roomStatusLabel.setHorizontalAlignment(SwingConstants.CENTER);
        roomStatusLabel.setFont(roomStatusLabel.getFont().deriveFont(Font.BOLD, 14f));
        panel.add(roomStatusLabel, BorderLayout.NORTH);

        // Players list
        playersArea = new JTextArea(8, 30);
        playersArea.setEditable(false);
        playersArea.setBackground(getBackground());
        playersArea.setFont(new Font(Font.MONOSPACED, Font.PLAIN, 12));
        playersScrollPane = new JScrollPane(playersArea);
        playersScrollPane.setVerticalScrollBarPolicy(JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED);
        panel.add(playersScrollPane, BorderLayout.CENTER);

        return panel;
    }

    private JPanel createControlPanel() {
        JPanel panel = new JPanel(new FlowLayout(FlowLayout.CENTER));

        startGameButton = new JButton("Start Game");
        startGameButton.addActionListener(this::onStartGame);
        startGameButton.setBackground(new Color(0, 150, 0));
        startGameButton.setForeground(Color.WHITE);
        startGameButton.setFont(startGameButton.getFont().deriveFont(Font.BOLD));

        leaveRoomButton = new JButton("Leave Room");
        leaveRoomButton.addActionListener(this::onLeaveRoom);

        panel.add(startGameButton);
        panel.add(leaveRoomButton);

        return panel;
    }

    private void onJoinRoom(ActionEvent e) {
        String roomId = roomIdField.getText().trim();
        if (roomId.isEmpty()) {
            roomId = null; // Join any available room
        }

        joinRoomButton.setEnabled(false);
        createRoomButton.setEnabled(false);

        networkManager.joinRoom(roomId);
    }

    private void onCreateRoom(ActionEvent e) {
        joinRoomButton.setEnabled(false);
        createRoomButton.setEnabled(false);

        // Create new room by joining without specifying room ID
        networkManager.joinRoom(null);
    }

    private void onStartGame(ActionEvent e) {
        if (!currentRoomId.isEmpty()) {
            startGameButton.setEnabled(false);
            networkManager.startGame();
        }
    }

    private void onLeaveRoom(ActionEvent e) {
        if (!currentRoomId.isEmpty()) {
            networkManager.leaveRoom();
        }
    }

    public void handleMessage(ProtocolMessage message) {
        switch (message.getType()) {
            case ROOM_JOINED:
                handleRoomJoined(message);
                break;

            case ROOM_LEFT:
                handleRoomLeft(message);
                break;

            case ERROR_MSG:
                handleError(message);
                break;

            default:
                // Ignore other messages in lobby
                break;
        }
    }

    private void handleRoomJoined(ProtocolMessage message) {
        String joiningPlayerId = message.getPlayerId();
        String joiningRoomId = message.getRoomId();
        String playerName = message.getData("player_name", joiningPlayerId);

        if (joiningPlayerId.equals(networkManager.getPlayerId())) {
            // I joined a room
            currentRoomId = joiningRoomId;

            // Only clear and reset if this is truly a fresh join (no existing players known)
            if (roomPlayers.isEmpty()) {
                roomPlayers.add(networkManager.getPlayerName());
            } else {
                // I already know about other players, just add myself if not already there
                if (!roomPlayers.contains(networkManager.getPlayerName())) {
                    roomPlayers.add(networkManager.getPlayerName());
                }
            }
            updateRoomState();
        } else if (joiningRoomId.equals(currentRoomId) || currentRoomId.isEmpty()) {
            // Someone else joined my room, OR I'm learning about existing players
            if (currentRoomId.isEmpty()) {
                currentRoomId = joiningRoomId;
            }

            if (!roomPlayers.contains(playerName)) {
                roomPlayers.add(playerName);
                updateRoomState();
            }
        }
    }

    private void handleRoomLeft(ProtocolMessage message) {
        if (message.getPlayerId().equals(networkManager.getPlayerId())) {
            // We left the room
            currentRoomId = "";
            roomPlayers.clear();
            updateRoomState();
        } else {
            // Another player left our room
            String playerName = getPlayerNameFromId(message.getPlayerId());
            roomPlayers.remove(playerName);
            updateRoomState();
        }
    }

    private void handleError(ProtocolMessage message) {
        // Re-enable buttons on error
        joinRoomButton.setEnabled(true);
        createRoomButton.setEnabled(true);
        startGameButton.setEnabled(true);
    }

    private void updateRoomState() {
        SwingUtilities.invokeLater(() -> {
            if (currentRoomId.isEmpty()) {
                // Not in a room
                roomStatusLabel.setText("Not in any room");
                playersArea.setText("Join or create a room to start playing!");

                joinRoomButton.setEnabled(true);
                createRoomButton.setEnabled(true);
                startGameButton.setEnabled(false);
                leaveRoomButton.setEnabled(false);

                roomIdField.setEnabled(true);
            } else {
                // In a room
                roomStatusLabel.setText("Room: " + currentRoomId);
                updatePlayersDisplay();

                joinRoomButton.setEnabled(false);
                createRoomButton.setEnabled(false);
                startGameButton.setEnabled(roomPlayers.size() >= 2);
                leaveRoomButton.setEnabled(true);

                roomIdField.setEnabled(false);
            }
        });
    }

    private void updatePlayersDisplay() {
        StringBuilder sb = new StringBuilder();
        sb.append("Players in room (").append(roomPlayers.size()).append("):\n\n");

        for (int i = 0; i < roomPlayers.size(); i++) {
            String playerName = roomPlayers.get(i);
            sb.append("  ").append(i + 1).append(". ").append(playerName);
            if (playerName.equals(networkManager.getPlayerName())) {
                sb.append(" (You)");
            }
            sb.append("\n");
        }

        if (roomPlayers.size() < 2) {
            sb.append("\nWaiting for more players...\n");
            sb.append("(Need at least 2 players to start)");
        } else {
            sb.append("\nReady to start game!");
        }

        playersArea.setText(sb.toString());
    }

    private String getPlayerNameFromId(String playerId) {
        // For simplicity, we'll use the player ID as name
        // In a more complex implementation, you might maintain a player ID -> name mapping
        return playerId;
    }

    // Public method to reset state when returning to lobby
    public void resetToLobby() {
        currentRoomId = "";
        roomPlayers.clear();
        updateRoomState();
    }
}