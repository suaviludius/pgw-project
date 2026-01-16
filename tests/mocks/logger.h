#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>

// Простые заглушки, которые выводят в std::cout и игнорируют агрументы фрматирования
#define LOG_INFO(msg, ...)    std::cout << "[INFO]  " << msg << std::endl
#define LOG_ERROR(msg, ...)   std::cout << "[ERROR] " << msg << std::endl
#define LOG_DEBUG(msg, ...)   std::cout << "[DEBUG] " << msg << std::endl
#define LOG_WARN(msg, ...)    std::cout << "[WARN]  " << msg << std::endl
#define LOG_TRACE(msg, ...)   std::cout << "[TRACE] " << msg << std::endl
#define LOG_CRITICAL(msg, ...) std::cout << "[CRIT]  " << msg << std::endl

#endif // LOGGER_H