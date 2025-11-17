//
// Created by chlad on 9/11/2025.
//

#include "Logger.h"
#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>

Logger::Logger(const std::string& filePath) {
    this->filePath = filePath;
    this->logToFile = true;           // Initialize first!
    this->logToConsole = false;        // Initialize first!
    this->logLevelMinimum = LogLevel::INFO;  // Initialize first!

    if (logToFile) {
        logFileStream.open(filePath, std::ios::app);
    }
}

Logger::~Logger() {
    if (logFileStream.is_open()) {
        logFileStream.close();
    }
}

void Logger::info(const std::string& message) {
    if (LogLevel::INFO < logLevelMinimum) {return;}
    writeToLog(LogLevel::INFO, message);
}

void Logger::debug(const std::string& message) {
    if (LogLevel::DEBUG < logLevelMinimum) {return;}
    writeToLog(LogLevel::DEBUG, message);
}

void Logger::error(const std::string& message) {
    if (LogLevel::ERROR < logLevelMinimum) {return;}
    writeToLog(LogLevel::ERROR, message);
}

void Logger::warning(const std::string& message) {
    if (LogLevel::WARNING < logLevelMinimum) {return;}
    writeToLog(LogLevel::WARNING, message);
}

void Logger::writeToLog(LogLevel logLevel, const std::string& message) {
    // CRITICAL: Lock the mutex first!
    std::string timestamp = getCurrentTimestamp();
    std::string level = levelToString(logLevel);
    std::string fullMessage = "[" + timestamp + "] " + level + ": " + message;
    std::lock_guard<std::mutex> lock(logMutex);
    if (logToConsole) {
        std::cout << fullMessage << std::endl;
    }
    if (logToFile) {
        logFileStream << fullMessage << std::endl;
        logFileStream.flush();  // Ensure it's written
    }
}

std::string Logger::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);

    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string Logger::levelToString(LogLevel logLevel) {
    switch (logLevel) {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR:   return "ERROR";
        default:                return "UNKNOWN";
    }
}

void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(logMutex);
    logLevelMinimum = level;
}

void Logger::setLogToFile(bool enabled) {
    std::lock_guard<std::mutex> lock(logMutex);
    logToFile = enabled;
}

void Logger::setLogToConsole(bool enabled) {
    std::lock_guard<std::mutex> lock(logMutex);
    logToConsole = enabled;
}