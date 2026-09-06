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

Logger::Logger() = default;

Logger::~Logger() = default;

void Logger::set_output_file(const std::string &path) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (file_stream_.is_open()) {
    file_stream_.close();
  }
  file_stream_.open(path, std::ios::app);
}

void Logger::log(Level level, const char *file, int line, const char *func,
                 const std::string &message) {
  if (level < level_)
    return;

  std::lock_guard<std::mutex> lock(mutex_);

  std::ostringstream oss;

  // Timestamp
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) %
            1000;

  std::tm tm = *std::localtime(&time_t);
  oss << '[' << std::put_time(&tm, "%H:%M:%S") << '.' << std::setfill('0')
      << std::setw(3) << ms.count() << "] ";

  // Level
  oss << '[' << level_string(level) << "] ";

  // File:line (function)
  oss << file << ':' << line << " (" << func << ") ";

  // Message
  oss << message;

  std::string output = oss.str();

  // Output to file if open
  if (file_stream_.is_open()) {
    file_stream_ << output << std::endl;
    file_stream_.flush();
  }

  // Output to console if enabled
  if (console_output_) {
    if (level >= Level::ERROR) {
      std::cerr << output << std::endl;
    } else {
      std::cout << output << std::endl;
    }
  }
}

std::string Logger::level_string(Level level) const {
  switch (level) {
  case Level::TRACE:
    return "TRACE";
  case Level::DEBUG:
    return "DEBUG";
  case Level::INFO:
    return "INFO ";
  case Level::WARN:
    return "WARN ";
  case Level::ERROR:
    return "ERROR";
  case Level::FATAL:
    return "FATAL";
  }
  return "UNKNOWN";
}

std::string Logger::timestamp() const {
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) %
            1000;

  std::tm tm = *std::localtime(&time_t);
  std::ostringstream oss;
  oss << std::put_time(&tm, "%H:%M:%S") << '.' << std::setfill('0')
      << std::setw(3) << ms.count();
  return oss.str();
}

} // namespace o2emu::logging