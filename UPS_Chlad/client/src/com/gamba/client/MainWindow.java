// MainWindow.java - Main application window
package com.gamba.client;

import javax.swing.*;
import java.awt.*;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;

public class MainWindow extends JFrame implements NetworkManager.MessageHandler {
    private final NetworkManager networkManager;

    // GUI Components
    private CardLayout cardLayout;
    private JPanel mainPanel;
    private LobbyPanel lobbyPanel;
    private GamePanel gamePanel;

    // Status bar
    private JLabel statusLabel;
    private JLabel connectionLabel;

    // Current state
    private ApplicationState currentState = ApplicationState.LOBBY;

    public MainWindow(NetworkManager networkManager) {
        this.networkManager = networkManager;
        this.networkManager.setMessageHandler(this);

        initComponents();
        setupWindow();
    }

    private void initComponents() {
        setTitle("Gamba Card Game - " + networkManager.getPlayerName());
        setDefaultCloseOperation(JFrame.DO_NOTHING_ON_CLOSE);
        setSize(900, 700);
        setLocationRelativeTo(null);

        // Add window listener for proper cleanup
        addWindowListener(new WindowAdapter() {
            @Override
            public void windowClosing(WindowEvent e) {
                handleWindowClosing();
            }
        });

        // Create main layout
        cardLayout = new CardLayout();
        mainPanel = new JPanel(cardLayout);

        // Create panels
        lobbyPanel = new LobbyPanel(networkManager);
        gamePanel = new GamePanel(networkManager);

        // Add panels to card layout
        mainPanel.add(lobbyPanel, ApplicationState.LOBBY.name());
        mainPanel.add(gamePanel, ApplicationState.GAME.name());

        add(mainPanel, BorderLayout.CENTER);

        // Create status bar
        createStatusBar();

        // Show initial state
        showState(ApplicationState.LOBBY);
    }

    private void createStatusBar() {
        JPanel statusBar = new JPanel(new BorderLayout());
        statusBar.setBorder(BorderFactory.createEtchedBorder());

        statusLabel = new JLabel("Ready");
        connectionLabel = new JLabel("Connected to " +
                networkManager.getNetworkAddress() + " as " + networkManager.getPlayerName());

        statusBar.add(statusLabel, BorderLayout.CENTER);
        statusBar.add(connectionLabel, BorderLayout.EAST);

        add(statusBar, BorderLayout.SOUTH);
    }

    private void setupWindow() {
        // Set icon if available
        try {
            // You can add an icon file to resources
            // ImageIcon icon = new ImageIcon(getClass().getResource("/gamba_icon.png"));
            // setIconImage(icon.getImage());
        } catch (Exception e) {
            // Icon not available, continue without it
        }
    }

    public void showState(ApplicationState state) {
        currentState = state;
        cardLayout.show(mainPanel, state.name());

        // Update status
        switch (state) {
            case LOBBY:
                setStatusText("In lobby - Select or create a room");
                break;
            case GAME:
                setStatusText("In game");
                break;
        }
    }

    public void setStatusText(String text) {
        SwingUtilities.invokeLater(() -> statusLabel.setText(text));
    }

    private void handleWindowClosing() {
        int option = JOptionPane.showConfirmDialog(
                this,
                "Are you sure you want to exit Gamba?",
                "Exit Confirmation",
                JOptionPane.YES_NO_OPTION,
                JOptionPane.QUESTION_MESSAGE
        );

        if (option == JOptionPane.YES_OPTION) {
            // Cleanup and exit
            networkManager.disconnect();
            dispose();
            System.exit(0);
        }
    }

    // NetworkManager.MessageHandler implementation
    @Override
    public void onMessageReceived(ProtocolMessage message) {
        SwingUtilities.invokeLater(() -> {
            try {
                handleMessage(message);
            } catch (Exception e) {
                System.err.println("Error handling message: " + e.getMessage());
                e.printStackTrace();
            }
        });
    }

    private void handleMessage(ProtocolMessage message) {
        switch (message.getType()) {
            case ROOM_JOINED:
                handleRoomJoined(message);
                break;

            case ROOM_LEFT:
                handleRoomLeft(message);
                break;

            case GAME_STARTED:
                handleGameStarted(message);
                break;

            case GAME_STATE:
                handleGameState(message);
                break;

            case GAME_OVER:
                handleGameOver(message);
                break;

            case ERROR_MSG:
                handleError(message);
                break;

            case PLAYER_DISCONNECTED:
                handlePlayerDisconnected(message);
                break;

            case TURN_RESULT:
                handleTurnResult(message);
                break;

            default:
                // Let individual panels handle other messages
                if (currentState == ApplicationState.LOBBY) {
                    lobbyPanel.handleMessage(message);
                } else if (currentState == ApplicationState.GAME) {
                    gamePanel.handleMessage(message);
                }
                break;
        }
    }

    private void handleRoomJoined(ProtocolMessage message) {
        if (message.getPlayerId().equals(networkManager.getPlayerId())) {
            setStatusText("Joined room " + message.getRoomId() + " - Waiting for other players...");
        }

        // Pass to current panel
        if (currentState == ApplicationState.LOBBY) {
            lobbyPanel.handleMessage(message);
        }
    }

    private void handleRoomLeft(ProtocolMessage message) {
        if (message.getPlayerId().equals(networkManager.getPlayerId())) {
            // We left the room - go back to lobby
            showState(ApplicationState.LOBBY);
            setStatusText("Left room - Back in lobby");
        }

        // Pass to current panel
        if (currentState == ApplicationState.GAME) {
            gamePanel.handleMessage(message);
        } else {
            lobbyPanel.handleMessage(message);
        }
    }

    private void handleGameStarted(ProtocolMessage message) {
        // Switch to game view
        showState(ApplicationState.GAME);
        setStatusText("Game started!");

        // Pass to game panel
        gamePanel.handleMessage(message);
    }

    private void handleGameState(ProtocolMessage message) {
        // Pass to game panel
        if (currentState == ApplicationState.GAME) {
            gamePanel.handleMessage(message);
        }
    }

    private void handleGameOver(ProtocolMessage message) {
        // Show game over dialog
        String winner = message.getData("winner", "Unknown");
        String result = message.getData("result", "unknown");
        String reason = message.getData("reason", "");

        String title = "Game Over";
        String messageText;

        if ("won".equals(result)) {
            messageText = "Congratulations! You won the game!";
        } else if ("lost".equals(result)) {
            messageText = "Game over. Winner: " + winner;
        } else {
            messageText = "Game ended. Result: " + result;
        }

        if (!reason.isEmpty() && reason.equals("opponent_disconnected")) {
            messageText += "\n(Opponent disconnected)";
        }

        JOptionPane.showMessageDialog(this, messageText, title,
                "won".equals(result) ? JOptionPane.INFORMATION_MESSAGE : JOptionPane.PLAIN_MESSAGE);

        // Pass to game panel for cleanup
        if (currentState == ApplicationState.GAME) {
            gamePanel.handleMessage(message);
        }

        // Will likely receive ROOM_LEFT message next to return to lobby
    }

    private void handleError(ProtocolMessage message) {
        String error = message.getData("error", "Unknown error");
        setStatusText("Error: " + error);

        JOptionPane.showMessageDialog(this, error, "Server Error", JOptionPane.ERROR_MESSAGE);
    }

    private void handlePlayerDisconnected(ProtocolMessage message) {
        String disconnectedPlayer = message.getData("departed_player", "Unknown player");
        setStatusText("Player " + disconnectedPlayer + " disconnected");

        // Pass to current panel
        if (currentState == ApplicationState.GAME) {
            gamePanel.handleMessage(message);
        }
    }

    private void handleTurnResult(ProtocolMessage message) {
        // Pass to game panel
        if (currentState == ApplicationState.GAME) {
            gamePanel.handleMessage(message);
        }
    }

    @Override
    public void onConnectionLost() {
        SwingUtilities.invokeLater(() -> {
            setStatusText("Connection lost!");

            int option = JOptionPane.showConfirmDialog(
                    this,
                    "Connection to server was lost. Would you like to exit?",
                    "Connection Lost",
                    JOptionPane.YES_NO_OPTION,
                    JOptionPane.WARNING_MESSAGE
            );

            if (option == JOptionPane.YES_OPTION) {
                dispose();
                System.exit(0);
            }
        });
    }
}

// Application states
enum ApplicationState {
    LOBBY, GAME
}