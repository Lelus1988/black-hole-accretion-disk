#pragma once

#include <iostream>
#include <sstream>

namespace Logger {

enum class Level {
    Info,
    Warning,
    Error
};

inline void log(Level level, const std::string& message) {
    const char* levelStr = "";
    switch (level) {
        case Level::Info:    levelStr = "[INFO]"; break;
        case Level::Warning: levelStr = "[WARN]"; break;
        case Level::Error:   levelStr = "[ERROR]"; break;
    }
    std::cout << levelStr << " " << message << std::endl;
}

inline void info(const std::string& message) {
    log(Level::Info, message);
}

inline void warning(const std::string& message) {
    log(Level::Warning, message);
}

inline void error(const std::string& message) {
    log(Level::Error, message);
}

} // namespace Logger
