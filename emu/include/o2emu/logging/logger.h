#pragma once

/**
 * @file logger.h
 * @brief Logging infrastructure
 */

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <o2emu/o2emu.h>
#include <sstream>
#include <string>

namespace o2emu::logging {

enum class Level : uint8_t {
  TRACE = 0,
  DEBUG = 1,
  INFO = 2,
  WARN = 3,
  ERROR = 4,
  FATAL = 5,
};

class Logger {
public:
  static Logger &instance() {
    static Logger logger;
    return logger;
  }

  // Set log level
  void set_level(Level level) { level_ = level; }
  Level level() const { return level_; }

  // Set output file
  void set_output_file(const std::string &path);

  // Enable/disable console output
  void set_console_output(bool enable) { console_output_ = enable; }

  // Log a message
  void log(Level level, const char *file, int line, const char *func,
           const std::string &message);

// Convenience macros
#define O2EMU_LOG_TRACE(msg)                                                   \
  o2emu::logging::Logger::instance().log(o2emu::logging::Level::TRACE,         \
                                         __FILE__, __LINE__, __func__, msg)
#define O2EMU_LOG_DEBUG(msg)                                                   \
  o2emu::logging::Logger::instance().log(o2emu::logging::Level::DEBUG,         \
                                         __FILE__, __LINE__, __func__, msg)
#define O2EMU_LOG_INFO(msg)                                                    \
  o2emu::logging::Logger::instance().log(o2emu::logging::Level::INFO,          \
                                         __FILE__, __LINE__, __func__, msg)
#define O2EMU_LOG_WARN(msg)                                                    \
  o2emu::logging::Logger::instance().log(o2emu::logging::Level::WARN,          \
                                         __FILE__, __LINE__, __func__, msg)
#define O2EMU_LOG_ERROR(msg)                                                   \
  o2emu::logging::Logger::instance().log(o2emu::logging::Level::ERROR,         \
                                         __FILE__, __LINE__, __func__, msg)
#define O2EMU_LOG_FATAL(msg)                                                   \
  o2emu::logging::Logger::instance().log(o2emu::logging::Level::FATAL,         \
                                         __FILE__, __LINE__, __func__, msg)

  // Formatted logging
  template <typename... Args>
  void logf(Level level, const char *file, int line, const char *func,
            const char *fmt, Args... args) {
    char buffer[4096];
    std::snprintf(buffer, sizeof(buffer), fmt, args...);
    log(level, file, line, func, buffer);
  }

#define O2EMU_LOG_TRACE_F(fmt, ...)                                            \
  o2emu::logging::Logger::instance().logf(o2emu::logging::Level::TRACE,        \
                                          __FILE__, __LINE__, __func__, fmt,   \
                                          ##__VA_ARGS__)
#define O2EMU_LOG_DEBUG_F(fmt, ...)                                            \
  o2emu::logging::Logger::instance().logf(o2emu::logging::Level::DEBUG,        \
                                          __FILE__, __LINE__, __func__, fmt,   \
                                          ##__VA_ARGS__)
#define O2EMU_LOG_INFO_F(fmt, ...)                                             \
  o2emu::logging::Logger::instance().logf(o2emu::logging::Level::INFO,         \
                                          __FILE__, __LINE__, __func__, fmt,   \
                                          ##__VA_ARGS__)
#define O2EMU_LOG_WARN_F(fmt, ...)                                             \
  o2emu::logging::Logger::instance().logf(o2emu::logging::Level::WARN,         \
                                          __FILE__, __LINE__, __func__, fmt,   \
                                          ##__VA_ARGS__)
#define O2EMU_LOG_ERROR_F(fmt, ...)                                            \
  o2emu::logging::Logger::instance().logf(o2emu::logging::Level::ERROR,        \
                                          __FILE__, __LINE__, __func__, fmt,   \
                                          ##__VA_ARGS__)
#define O2EMU_LOG_FATAL_F(fmt, ...)                                            \
  o2emu::logging::Logger::instance().logf(o2emu::logging::Level::FATAL,        \
                                          __FILE__, __LINE__, __func__, fmt,   \
                                          ##__VA_ARGS__)

private:
  Logger();
  ~Logger();

  Level level_ = Level::INFO;
  bool console_output_ = true;
  std::ofstream file_stream_;
  std::mutex mutex_;

  std::string level_string(Level level) const;
  std::string timestamp() const;
};

} // namespace o2emu::logging