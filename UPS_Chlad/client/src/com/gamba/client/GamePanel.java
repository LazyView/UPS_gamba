// GamePanel.java - Main game interface
package com.gamba.client;

import javax.swing.*;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class GamePanel extends JPanel {
    private final NetworkManager networkManager;

    // Game state
    private String currentPlayer = "";
    private String topCard = "";
    private List<String> hand = new ArrayList<>();
    private int reserveCount = 0;
    private boolean isMyTurn = false;
    private boolean deckEmpty = false;
    private int discardSize = 0;

    // Selected cards for playing
    private List<String> selectedCards = new ArrayList<>();

    // GUI Components
    private JLabel gameStatusLabel;
    private JLabel topCardLabel;
    private JLabel deckStatusLabel;
    private JPanel handPanel;
    private JPanel controlPanel;
    private JButton playCardsButton;
    private JButton pickupPileButton;
    private JButton playReserveButton;
    private JTextArea gameLogArea;
    private JScrollPane logScrollPane;

    // Card display
    private List<CardButton> cardButtons = new ArrayList<>();

    public GamePanel(NetworkManager networkManager) {
        this.networkManager = networkManager;
        initComponents();
    }

    private void initComponents() {
        setLayout(new BorderLayout());
        setBorder(BorderFactory.createTitledBorder("Gamba Game"));

        // Create top panel (game status)
        JPanel topPanel = createTopPanel();
        add(topPanel, BorderLayout.NORTH);

        // Create center panel (game board)
        JPanel centerPanel = createCenterPanel();
        add(centerPanel, BorderLayout.CENTER);

        // Create bottom panel (player controls)
        JPanel bottomPanel = createBottomPanel();
        add(bottomPanel, BorderLayout.SOUTH);

        // Initial state
        updateGameState();
    }

    private JPanel createTopPanel() {
        JPanel panel = new JPanel(new GridLayout(2, 1));

        // Game status
        gameStatusLabel = new JLabel("Waiting for game to start...", SwingConstants.CENTER);
        gameStatusLabel.setFont(gameStatusLabel.getFont().deriveFont(Font.BOLD, 16f));
        panel.add(gameStatusLabel);

        // Top card and deck status
        JPanel statusPanel = new JPanel(new FlowLayout());
        topCardLabel = new JLabel("Top Card: None");
        deckStatusLabel = new JLabel("Deck: Full");

        statusPanel.add(topCardLabel);
        statusPanel.add(Box.createHorizontalStrut(20));
        statusPanel.add(deckStatusLabel);

        panel.add(statusPanel);

        return panel;
    }

    private JPanel createCenterPanel() {
        JPanel panel = new JPanel(new BorderLayout());

        // Hand display
        JPanel handContainer = new JPanel(new BorderLayout());
        handContainer.setBorder(BorderFactory.createTitledBorder("Your Hand"));

        handPanel = new JPanel(new FlowLayout(FlowLayout.LEFT));
        JScrollPane handScrollPane = new JScrollPane(handPanel);
        handScrollPane.setPreferredSize(new Dimension(0, 120));
        handScrollPane.setHorizontalScrollBarPolicy(JScrollPane.HORIZONTAL_SCROLLBAR_AS_NEEDED);
        handScrollPane.setVerticalScrollBarPolicy(JScrollPane.VERTICAL_SCROLLBAR_NEVER);

        handContainer.add(handScrollPane, BorderLayout.CENTER);
        panel.add(handContainer, BorderLayout.NORTH);

        // Game log
        JPanel logContainer = new JPanel(new BorderLayout());
        logContainer.setBorder(BorderFactory.createTitledBorder("Game Log"));

        gameLogArea = new JTextArea(10, 30);
        gameLogArea.setEditable(false);
        gameLogArea.setFont(new Font(Font.MONOSPACED, Font.PLAIN, 12));
        logScrollPane = new JScrollPane(gameLogArea);
        logScrollPane.setVerticalScrollBarPolicy(JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED);

        logContainer.add(logScrollPane, BorderLayout.CENTER);
        panel.add(logContainer, BorderLayout.CENTER);

        return panel;
    }

    private JPanel createBottomPanel() {
        JPanel panel = new JPanel(new BorderLayout());

        // Control buttons
        controlPanel = new JPanel(new FlowLayout());

        playCardsButton = new JButton("Play Selected Cards");
        playCardsButton.addActionListener(this::onPlayCards);
        playCardsButton.setEnabled(false);

        pickupPileButton = new JButton("Pick Up Pile");
        pickupPileButton.addActionListener(this::onPickupPile);
        pickupPileButton.setEnabled(false);

        playReserveButton = new JButton("Play Reserve Card (Blind)");
        playReserveButton.addActionListener(this::onPlayReserve);
        playReserveButton.setEnabled(false);
        playReserveButton.setBackground(new Color(150, 100, 0));
        playReserveButton.setForeground(Color.WHITE);

        controlPanel.add(playCardsButton);
        controlPanel.add(pickupPileButton);
        controlPanel.add(playReserveButton);

        // Selection info
        JLabel selectionLabel = new JLabel("Click cards to select them for playing");
        selectionLabel.setHorizontalAlignment(SwingConstants.CENTER);

        panel.add(selectionLabel, BorderLayout.NORTH);
        panel.add(controlPanel, BorderLayout.CENTER);

        return panel;
    }

    private void onPlayCards(ActionEvent e) {
        if (selectedCards.isEmpty()) {
            addToLog("No cards selected!");
            return;
        }

        playCardsButton.setEnabled(false);
        networkManager.playCards(selectedCards.toArray(new String[0]));
        addToLog("Playing: " + String.join(", ", selectedCards));
    }

    private void onPickupPile(ActionEvent e) {
        pickupPileButton.setEnabled(false);
        networkManager.pickupPile();
        addToLog("Picking up discard pile (" + discardSize + " cards)");
    }

    private void onPlayReserve(ActionEvent e) {
        playReserveButton.setEnabled(false);
        networkManager.playCards("RESERVE");
        addToLog("Playing reserve card (blind)");
    }

    public void handleMessage(ProtocolMessage message) {
        switch (message.getType()) {
            case GAME_STARTED:
                handleGameStarted(message);
                break;

            case GAME_STATE:
                handleGameState(message);
                break;

            case TURN_RESULT:
                handleTurnResult(message);
                break;

            case GAME_OVER:
                handleGameOver(message);
                break;

            case PLAYER_DISCONNECTED:
                handlePlayerDisconnected(message);
                break;

            default:
                // Ignore other messages in game
                break;
        }
    }

    private void handleGameStarted(ProtocolMessage message) {
        addToLog("=== GAME STARTED ===");
        addToLog("Players: " + message.getData("players", "?"));
        addToLog("Current player: " + message.getData("current_player", "?"));
        addToLog("Starting card: " + message.getData("top_card", "?"));
    }

    private void handleGameState(ProtocolMessage message) {
        // Update game state
        currentPlayer = message.getData("current_player", "");
        topCard = message.getData("top_card", "");
        deckEmpty = "true".equals(message.getData("deck_empty", "false"));
        discardSize = Integer.parseInt(message.getData("discard_size", "0"));
        isMyTurn = "true".equals(message.getData("is_turn", "false"));

        // Parse hand
        String handStr = message.getData("hand", "[]");
        parseHand(handStr);

        // Parse reserve count
        reserveCount = Integer.parseInt(message.getData("reserve_count", "0"));

        // Update display
        updateGameState();
        updateHandDisplay();

        // Log important state changes
        if (isMyTurn) {
            addToLog(">>> YOUR TURN <<<");
        }
    }

    private void handleTurnResult(ProtocolMessage message) {
        String result = message.getData("result", "unknown");

        switch (result) {
            case "0": // SUCCESS
                addToLog("Card play successful!");
                clearSelection();
                break;

            case "1": // INVALID_PLAYER
                addToLog("ERROR: Not your turn!");
                break;

            case "2": // INVALID_CARD
                addToLog("ERROR: Invalid card selection!");
                break;

            case "4": // PICKUP_REQUIRED
                addToLog("Cannot play that card - you must pick up the pile!");
                break;

            case "pickup_success":
                addToLog("Successfully picked up the pile!");
                break;

            default:
                addToLog("Turn result: " + result);
                break;
        }

        // Re-enable buttons
        updateButtons();
    }

    private void handleGameOver(ProtocolMessage message) {
        String winner = message.getData("winner", "Unknown");
        String result = message.getData("result", "unknown");

        addToLog("=== GAME OVER ===");
        addToLog("Winner: " + winner);
        addToLog("Your result: " + result);

        // Disable all game controls
        setGameControlsEnabled(false);
        clearSelection();
    }

    private void handlePlayerDisconnected(ProtocolMessage message) {
        String departedPlayer = message.getData("departed_player", "Unknown");
        addToLog("Player " + departedPlayer + " disconnected!");
    }

    private void parseHand(String handStr) {
        hand.clear();

        // Remove brackets and split by comma
        if (handStr.startsWith("[") && handStr.endsWith("]")) {
            handStr = handStr.substring(1, handStr.length() - 1);
        }

        if (!handStr.trim().isEmpty()) {
            String[] cards = handStr.split(",");
            for (String card : cards) {
                hand.add(card.trim());
            }
        }
    }

    private void updateGameState() {
        SwingUtilities.invokeLater(() -> {
            // Update status labels
            if (isMyTurn) {
                gameStatusLabel.setText("YOUR TURN");
                gameStatusLabel.setForeground(new Color(0, 150, 0));
            } else {
                gameStatusLabel.setText("Current player: " + currentPlayer);
                gameStatusLabel.setForeground(Color.BLACK);
            }

            topCardLabel.setText("Top Card: " + topCard + " | Discard Pile: " + discardSize + " cards");
            deckStatusLabel.setText("Deck: " + (deckEmpty ? "EMPTY (Phase 2)" : "Available") +
                    " | Reserve Cards: " + reserveCount);

            updateButtons();
        });
    }

    private void updateHandDisplay() {
        SwingUtilities.invokeLater(() -> {
            handPanel.removeAll();
            cardButtons.clear();

            for (String card : hand) {
                CardButton cardButton = new CardButton(card);
                cardButton.addActionListener(e -> toggleCardSelection(cardButton));
                cardButtons.add(cardButton);
                handPanel.add(cardButton);
            }

            handPanel.revalidate();
            handPanel.repaint();
        });
    }

    private void toggleCardSelection(CardButton cardButton) {
        String card = cardButton.getCard();

        if (selectedCards.contains(card)) {
            selectedCards.remove(card);
            cardButton.setSelected(false);
        } else {
            selectedCards.add(card);
            cardButton.setSelected(true);
        }

        updateButtons();

        // Log selection
        if (selectedCards.isEmpty()) {
            addToLog("No cards selected");
        } else {
            addToLog("Selected: " + String.join(", ", selectedCards));
        }
    }

    private void clearSelection() {
        selectedCards.clear();
        for (CardButton button : cardButtons) {
            button.setSelected(false);
        }
        updateButtons();
    }

    private void updateButtons() {
        boolean canAct = isMyTurn;
        boolean hasSelection = !selectedCards.isEmpty();
        boolean hasHand = !hand.isEmpty();
        boolean hasReserve = reserveCount > 0 && hand.isEmpty();

        playCardsButton.setEnabled(canAct && hasSelection);
        pickupPileButton.setEnabled(canAct && discardSize > 0);
        playReserveButton.setEnabled(canAct && hasReserve);
    }

    private void setGameControlsEnabled(boolean enabled) {
        playCardsButton.setEnabled(enabled);
        pickupPileButton.setEnabled(enabled);
        playReserveButton.setEnabled(enabled);
    }

    private void addToLog(String message) {
        SwingUtilities.invokeLater(() -> {
            gameLogArea.append(message + "\n");
            gameLogArea.setCaretPosition(gameLogArea.getDocument().getLength());
        });
    }

    // Custom card button class
    private static class CardButton extends JButton {
        private final String card;
        private boolean selected = false;

        public CardButton(String card) {
            this.card = card;
            setText(card);
            setPreferredSize(new Dimension(80, 60));
            setFont(getFont().deriveFont(Font.BOLD, 12f));
            updateAppearance();
        }

        public String getCard() {
            return card;
        }

        public void setSelected(boolean selected) {
            this.selected = selected;
            updateAppearance();
        }

        private void updateAppearance() {
            if (selected) {
                setBackground(new Color(100, 150, 255));
                setForeground(Color.WHITE);
                setBorder(BorderFactory.createLineBorder(Color.BLUE, 2));
            } else {
                setBackground(Color.WHITE);
                setForeground(Color.BLACK);
                setBorder(BorderFactory.createLineBorder(Color.GRAY, 1));
            }
        }
    }
}