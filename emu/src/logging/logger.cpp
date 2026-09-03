/**
 * @file logger.cpp
 * @brief Logging implementation
 */

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <o2emu/logging/logger.h>
#include <sstream>

namespace o2emu::logging {

static std::mutex log_mutex;
static LogLevel current_level = LogLevel::Info;
static bool show_timestamp = true;
static bool show_level = true;
static bool show_category = true;

void Logger::set_level(LogLevel level) {
  std::lock_guard<std::mutex> lock(log_mutex);
  current_level = level;
}

LogLevel Logger::level() { return current_level; }

void Logger::set_timestamp(bool enable) { show_timestamp = enable; }

void Logger::set_show_level(bool enable) { show_level = enable; }

void Logger::set_show_category(bool enable) { show_category = enable; }

void Logger::log(LogLevel level, const std::string &category,
                 const std::string &message) {
  if (level < current_level)
    return;

  std::lock_guard<std::mutex> lock(log_mutex);

  std::ostringstream oss;

  if (show_timestamp) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) %
              1000;

    std::tm tm = *std::localtime(&time_t);
    oss << '[' << std::put_time(&tm, "%H:%M:%S") << '.' << std::setfill('0')
        << std::setw(3) << ms.count() << "] ";
  }

  if (show_level) {
    const char *level_str = "";
    switch (level) {
    case LogLevel::Trace:
      level_str = "TRACE";
      break;
    case LogLevel::Debug:
      level_str = "DEBUG";
      break;
    case LogLevel::Info:
      level_str = "INFO ";
      break;
    case LogLevel::Warn:
      level_str = "WARN ";
      break;
    case LogLevel::Error:
      level_str = "ERROR";
      break;
    case LogLevel::Fatal:
      level_str = "FATAL";
      break;
    }
    oss << '[' << level_str << "] ";
  }

  if (show_category && !category.empty()) {
    oss << '[' << category << "] ";
  }

  oss << message;

  // Output to stderr for errors and above, stdout for others
  if (level >= LogLevel::Error) {
    std::cerr << oss.str() << std::endl;
  } else {
    std::cout << oss.str() << std::endl;
  }
}

void Logger::trace(const std::string &category, const std::string &message) {
  log(LogLevel::Trace, category, message);
}

void Logger::debug(const std::string &category, const std::string &message) {
  log(LogLevel::Debug, category, message);
}

void Logger::info(const std::string &category, const std::string &message) {
  log(LogLevel::Info, category, message);
}

void Logger::warn(const std::string &category, const std::string &message) {
  log(LogLevel::Warn, category, message);
}

void Logger::error(const std::string &category, const std::string &message) {
  log(LogLevel::Error, category, message);
}

void Logger::fatal(const std::string &category, const std::string &message) {
  log(LogLevel::Fatal, category, message);
}

} // namespace o2emu::logging