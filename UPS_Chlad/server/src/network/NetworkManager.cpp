//
// Created by chlad on 9/15/2025.
//

#include "NetworkManager.h"
// TODO: Add other includes you need

// TODO: Constructor implementation
NetworkManager::NetworkManager(/* your parameters */) {
    // Initialize members
}

// TODO: Destructor implementation
NetworkManager::~NetworkManager() {
    // Call cleanup if needed
}

// TODO: Implement start() method
bool NetworkManager::start() {
    // Call setupSocket()
    // Set running = true
    // Return success/failure
}

// TODO: Implement run() method
void NetworkManager::run() {
    // Main accept() loop
    // For each client: create thread calling handleClient()
    // Handle graceful shutdown
}

// TODO: Implement stop() method
void NetworkManager::stop() {
    // Set running = false
    // Close server socket
    // Call cleanup()
}

// TODO: Implement setupSocket() method
bool NetworkManager::setupSocket() {
    // socket() - create socket
    // setsockopt() - set SO_REUSEADDR
    // bind() - bind to IP:port
    // listen() - start listening
    // Return success/failure
}

// TODO: Implement handleClient() method
void NetworkManager::handleClient(int client_socket) {
    // recv() loop to get messages
    // Use MessageHandler to process messages
    // send() responses back
    // Handle client disconnect
    // Clean up client socket
}

// TODO: Implement cleanup() method
void NetworkManager::cleanup() {
    // Close sockets, cleanup resources
}