#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>

#ifndef TEST_LOG
    // Полное игнорирование
    #define LOG_INFO(msg, ...)      ((void)0)
    #define LOG_DEBUG(msg, ...)     ((void)0)
    #define LOG_ERROR(msg, ...)     ((void)0)
    #define LOG_WARN(msg, ...)      ((void)0)
    #define LOG_TRACE(msg, ...)     ((void)0)
    #define LOG_CRITICAL(msg, ...)  std::cout << "[CRIT]  " << msg << std::endl
#else
    // Простые заглушки, которые выводят в std::cout и игнорируют агрументы фрматирования
    #define LOG_INFO(msg, ...)      std::cout << "[INFO]  " << msg << std::endl
    #define LOG_ERROR(msg, ...)     std::cout << "[ERROR] " << msg << std::endl
    #define LOG_DEBUG(msg, ...)     std::cout << "[DEBUG] " << msg << std::endl
    #define LOG_WARN(msg, ...)      std::cout << "[WARN]  " << msg << std::endl
    #define LOG_TRACE(msg, ...)     std::cout << "[TRACE] " << msg << std::endl
    #define LOG_CRITICAL(msg, ...)  std::cout << "[CRIT]  " << msg << std::endl
#endif

#endif // LOGGER_H
