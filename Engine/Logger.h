#pragma once

#include "pch.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <format>

namespace JEngine {

enum class LogLevel { INFO, WARNING, ERROR, DEBUG };

// Singleton-based logging system that writes to both console and log.txt file.
// Supports type-safe logging using C++20 std::format.
//
// NOTE: Designed for single-threaded environments only. (Current Version)
//   TODO: Add thread-safety features (e.g., mutex) for multi-threaded use cases.
// 
// Example usage:
//   using namespace JEngine;
//
//   // 1. std::format-based logging (recommended)
//   LogInfo("Application started");
//   LogInfo("Player ID: {}, Name: {}", 123, "Player1");
//   LogWarning("Memory usage: {}%", 85);
//   LogError("Failed to load file: {}", "texture.png");
//   LogDebug("Frame time: {} ms", 16.67f);
//
//   // 2. formatToStream-based logging (flexible type mixing)
//   LogMultiple(LogLevel::INFO, "Entity", 42, "HP", 100, "Position", 10.5f, 20.3f);
//   // Output: [INFO] Entity 42 HP 100 Position 10.5 20.3
//
//   // 3. Exit on critical error
//   ExitWithMessage("Critical error: {}", errorCode);
//
// Output example:
//   [INFO] Application started
//   [INFO] Player ID: 123, Name: Player1
//   [WARNING] Memory usage: 85%
//   [ERROR] Failed to load file: texture.png
//   [DEBUG] Frame time: 16.67 ms
//   [INFO] Entity 42 HP 100 Position 10.5 20.3
//
// WARNING: Statistics are automatically written to log.txt on program termination.
class Logger
{
  private:
    Logger() : messagesProcessed(0) {
        // Open log file (out: write mode, trunc: overwrite mode)
        logFile.open("log.txt", std::ios::out | std::ios::trunc);

        if (!logFile.is_open()) {
            // cerr is the error output stream
            std::cerr << "ERROR: Could not open log.txt for writing!" << std::endl;
        }
    }

    // Delete Copy Constructor and Assignment Operator
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // template<typename T>
    // void formatToStream(std::ostringstream& oss, T&& arg) {
    //     // forward를 사용하여 인자를 그대로 전달
    //     oss << std::forward<T>(arg);
    // }

    // template<typename T, typename... Args>
    // void formatToStream(std::ostringstream& oss, T&& firstArg, Args&&... args) {
    //     oss << std::forward<T>(firstArg);
    //     // args의 개수가 0보다 큰지 확인
    //     if (sizeof...(args) > 0) {
    //         // 각 인자 사이에 공백 추가
    //         oss << " ";
    //         // 가변 길이 템플릿을 사용해 재귀적으로 처리
    //         formatToStream(oss, std::forward<Args>(args)...);
    //     }
    // }

    // Using Fold Expressiong (C++ 17 and later)
    template <typename T, typename... Args>
    void formatToStream(std::ostringstream& oss, T&& firstArg, Args&&... args) {
        oss << std::forward<T>(firstArg);
        if constexpr (sizeof...(args) > 0) {
            ((oss << " " << std::forward<Args>(args)), ...);
        }
    }

    std::string getLogLevelString(LogLevel level) {
        switch (level) {
        case LogLevel::INFO:
            return "[INFO]";
        case LogLevel::WARNING:
            return "[WARNING]";
        case LogLevel::ERROR:
            return "[ERROR]";
        case LogLevel::DEBUG:
            return "[DEBUG]";
        default:
            return "[UNKNOWN]";
        }
    }

  public:
    ~Logger() {
        if (logFile.is_open()) {
            // Write final statistics
            logFile << "\n=== Logging Session Ended ===" << std::endl;
            logFile << "Total messages processed: " << messagesProcessed << std::endl;
            logFile.flush();
            logFile.close();
        }
    }

    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    // Writes a log message to console and file.
    static void printLog(const std::string& message, LogLevel level = LogLevel::INFO) {
        auto& logger = getInstance();

        std::string levelStr = logger.getLogLevelString(level);
        std::string fullMessage = std::format("{} {}", levelStr, message);

        std::cout << fullMessage << std::endl;

        if (logger.logFile.is_open()) {
            logger.logFile << fullMessage << std::endl;
            logger.logFile.flush();
            logger.messagesProcessed++;
        } else {
            std::cerr << "ERROR: Log file is not open! Current Message lost: " << message
                      << std::endl;
        }
    }

    // Logs variadic arguments using formatToStream.
    template <typename... Args>
    static void log(LogLevel level, Args&&... args) { 
        auto& logger = getInstance();

        // Concatenate multiple arguments into a single string
        std::ostringstream oss;
        logger.formatToStream(oss, std::forward<Args>(args)...);
        std::string message = oss.str();

        std::string levelStr = logger.getLogLevelString(level);
        std::string fullMessage = std::format("{} {}", levelStr, message);

        std::cout << fullMessage << std::endl;

        if (logger.logFile.is_open()) {
            logger.logFile << fullMessage << std::endl;
            logger.messagesProcessed++;
        } else {
            std::cerr << "ERROR: Log file is not open! Current Message lost: " << message
                      << std::endl;
        }
    }

  private:
    std::ofstream logFile;
    size_t messagesProcessed;
};

// Logs an INFO-level message using std::format.
// Example: LogInfo("Player spawned at ({}, {})", x, y);
template <typename... Args>
void LogInfo(std::format_string<Args...> fmt, Args&&... args) {
    std::string message = std::format(fmt, std::forward<Args>(args)...);
    Logger::printLog(message, LogLevel::INFO);
}

// Logs a WARNING-level message using std::format.
// Example: LogWarning("Low memory: {} MB", availableMemory);
template <typename... Args>
void LogWarning(std::format_string<Args...> fmt, Args&&... args) {
    std::string message = std::format(fmt, std::forward<Args>(args)...);
    Logger::printLog(message, LogLevel::WARNING);
}

// Logs an ERROR-level message using std::format.
// Example: LogError("Failed to load asset: {}", filename);
template <typename... Args>
void LogError(std::format_string<Args...> fmt, Args&&... args) {
    std::string message = std::format(fmt, std::forward<Args>(args)...);
    Logger::printLog(message, LogLevel::ERROR);
}

// Logs a DEBUG-level message using std::format.
// Example: LogDebug("FPS: {}, Frame time: {} ms", fps, frameTime);
template <typename... Args>
void LogDebug(std::format_string<Args...> fmt, Args&&... args) {
    std::string message = std::format(fmt, std::forward<Args>(args)...);
    Logger::printLog(message, LogLevel::DEBUG);
}

// Logs multiple arguments separated by spaces using formatToStream.
// More flexible than std::format for mixing different types.
// Example: LogMultiple(LogLevel::INFO, "Entity", entityId, "HP", hp, "Mana", mana);
template <typename... Args>
void LogMultiple(LogLevel level, Args&&... args) {
    Logger::log(level, std::forward<Args>(args)...);
}

// Logs an error message and terminates the program.
// Calls assert(false) and std::exit(EXIT_FAILURE).
// Example: ExitWithMessage("Fatal error: {}", errorMessage);
template <typename... Args>
void ExitWithMessage(std::format_string<Args...> fmt, Args&&... args) {
    std::string message = std::format(fmt, std::forward<Args>(args)...);
    Logger::printLog(message, LogLevel::ERROR);
    assert(false);
    std::exit(EXIT_FAILURE);
}

} // namespace JEngine