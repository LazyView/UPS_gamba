// GambaClient.java - Main client application entry point
package com.gamba.client;

import javax.swing.*;
import java.awt.*;

public class GambaClient {
    private static final String VERSION = "1.0";

    public static void main(String[] args) {
        // Set look and feel
        try {
            UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
        } catch (Exception e) {
            System.err.println("Could not set system look and feel: " + e.getMessage());
        }

        // Create and start the application
        SwingUtilities.invokeLater(() -> {
            try {
                new GambaClient().start();
            } catch (Exception e) {
                e.printStackTrace();
                JOptionPane.showMessageDialog(null,
                        "Failed to start Gamba Client: " + e.getMessage(),
                        "Startup Error", JOptionPane.ERROR_MESSAGE);
            }
        });
    }

    private void start() {
        // Show connection dialog first
        ConnectionDialog connectionDialog = new ConnectionDialog();
        connectionDialog.setVisible(true);

        // If connection successful, proceed to main application
        if (connectionDialog.isConnected()) {
            NetworkManager networkManager = connectionDialog.getNetworkManager();

            // Create and show main application window
            MainWindow mainWindow = new MainWindow(networkManager);
            mainWindow.setVisible(true);

            // Dispose connection dialog
            connectionDialog.dispose();
        } else {
            // User cancelled or connection failed
            System.exit(0);
        }
    }
}

// ConnectionDialog.java - Initial connection setup
class ConnectionDialog extends JDialog {
    private JTextField hostField;
    private JTextField portField;
    private JTextField nameField;
    private NetworkManager networkManager;
    private boolean connected = false;

    public ConnectionDialog() {
        setTitle("Connect to Gamba Server");
        setModal(true);
        setDefaultCloseOperation(DO_NOTHING_ON_CLOSE);
        setResizable(false);

        initComponents();
        pack();
        setLocationRelativeTo(null);
    }

    private void initComponents() {
        setLayout(new BorderLayout());

        // Create input panel
        JPanel inputPanel = new JPanel(new GridBagLayout());
        GridBagConstraints gbc = new GridBagConstraints();
        gbc.insets = new Insets(5, 5, 5, 5);

        // Host input
        gbc.gridx = 0; gbc.gridy = 0; gbc.anchor = GridBagConstraints.EAST;
        inputPanel.add(new JLabel("Server Host:"), gbc);
        gbc.gridx = 1; gbc.fill = GridBagConstraints.HORIZONTAL; gbc.weightx = 1.0;
        hostField = new JTextField("localhost", 15);
        inputPanel.add(hostField, gbc);

        // Port input
        gbc.gridx = 0; gbc.gridy = 1; gbc.fill = GridBagConstraints.NONE; gbc.weightx = 0;
        inputPanel.add(new JLabel("Server Port:"), gbc);
        gbc.gridx = 1; gbc.fill = GridBagConstraints.HORIZONTAL; gbc.weightx = 1.0;
        portField = new JTextField("8080", 15);
        inputPanel.add(portField, gbc);

        // Name input
        gbc.gridx = 0; gbc.gridy = 2; gbc.fill = GridBagConstraints.NONE; gbc.weightx = 0;
        inputPanel.add(new JLabel("Player Name:"), gbc);
        gbc.gridx = 1; gbc.fill = GridBagConstraints.HORIZONTAL; gbc.weightx = 1.0;
        nameField = new JTextField("Player", 15);
        inputPanel.add(nameField, gbc);

        add(inputPanel, BorderLayout.CENTER);

        // Create button panel
        JPanel buttonPanel = new JPanel(new FlowLayout());
        JButton connectButton = new JButton("Connect");
        JButton cancelButton = new JButton("Cancel");

        connectButton.addActionListener(e -> attemptConnection());
        cancelButton.addActionListener(e -> {
            connected = false;
            dispose();
        });

        buttonPanel.add(connectButton);
        buttonPanel.add(cancelButton);
        add(buttonPanel, BorderLayout.SOUTH);

        // Set default button
        getRootPane().setDefaultButton(connectButton);
    }

    private void attemptConnection() {
        String host = hostField.getText().trim();
        String portText = portField.getText().trim();
        String name = nameField.getText().trim();

        // Validate input
        if (host.isEmpty() || portText.isEmpty() || name.isEmpty()) {
            JOptionPane.showMessageDialog(this,
                    "Please fill in all fields", "Input Error", JOptionPane.ERROR_MESSAGE);
            return;
        }

        int port;
        try {
            port = Integer.parseInt(portText);
            if (port < 1 || port > 65535) {
                throw new NumberFormatException("Port out of range");
            }
        } catch (NumberFormatException e) {
            JOptionPane.showMessageDialog(this,
                    "Please enter a valid port number (1-65535)", "Input Error", JOptionPane.ERROR_MESSAGE);
            return;
        }

        // Attempt connection
        setCursor(Cursor.getPredefinedCursor(Cursor.WAIT_CURSOR));

        try {
            networkManager = new NetworkManager(host, port);

            // Connect and send initial connect message
            if (networkManager.connect(name)) {
                connected = true;
                setCursor(Cursor.getDefaultCursor());
                dispose();
            } else {
                setCursor(Cursor.getDefaultCursor());
                JOptionPane.showMessageDialog(this,
                        "Failed to connect to server", "Connection Error", JOptionPane.ERROR_MESSAGE);
            }
        } catch (Exception e) {
            setCursor(Cursor.getDefaultCursor());
            JOptionPane.showMessageDialog(this,
                    "Connection failed: " + e.getMessage(), "Connection Error", JOptionPane.ERROR_MESSAGE);
        }
    }

    public boolean isConnected() {
        return connected;
    }

    public NetworkManager getNetworkManager() {
        return networkManager;
    }
}