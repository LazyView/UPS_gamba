#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3
};

class Logger {
private:
    std::string filePath;
    std::ofstream logFileStream;
    std::mutex logMutex;
    LogLevel logLevelMinimum;
    bool logToFile;
    bool logToConsole;

public:
    Logger(const std::string& filePath = "gamba_server.log");
    ~Logger();
    void setLogLevel(LogLevel logLevelMinumum);
    void setLogToFile(bool logToFile);
    void setLogToConsole(bool logToConsole);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    void debug(const std::string& message);

private:
    void writeToLog(LogLevel logLevel, const std::string& message);
    std::string getCurrentTimestamp();
    std::string levelToString(LogLevel logLevel);
};

#endif // LOGGER_H