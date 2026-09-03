#pragma once

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <string_view>

namespace o2emu {

enum class LogLevel : uint8_t {
  Trace = 0,
  Debug = 1,
  Info = 2,
  Warn = 3,
  Error = 4,
  Fatal = 5,
};

enum class LogCategory : uint16_t {
  General = 0x0001,
  CPU = 0x0002,
  Memory = 0x0004,
  Bus = 0x0008,
  CRIME = 0x0010,
  RE = 0x0020,
  GBE = 0x0040,
  MACE = 0x0080,
  VICE = 0x0100,
  PCI = 0x0200,
  SCSI = 0x0400,
  Ethernet = 0x0800,
  Audio = 0x1000,
  Video = 0x2000,
  PROM = 0x4000,
  All = 0x7fff,
};

struct LogEntry {
  LogLevel level;
  LogCategory category;
  std::chrono::steady_clock::time_point timestamp;
  uint64_t cycle_count;
  std::string_view message;
  std::string_view file;
  int line;
  std::string_view function;
};

using LogCallback = std::function<void(const LogEntry &)>;

class Logger {
public:
  static Logger &instance() {
    static Logger logger;
    return logger;
  }

  void set_level(LogLevel level) { min_level_ = level; }
  void set_categories(LogCategory cats) { categories_ = cats; }
  void add_category(LogCategory cat) {
    categories_ = static_cast<LogCategory>(static_cast<uint16_t>(categories_) |
                                           static_cast<uint16_t>(cat));
  }
  void remove_category(LogCategory cat) {
    categories_ = static_cast<LogCategory>(static_cast<uint16_t>(categories_) &
                                           ~static_cast<uint16_t>(cat));
  }

  void set_callback(LogCallback cb) { callback_ = std::move(cb); }
  void set_output_file(const std::string &path);
  void set_cycle_counter(std::function<uint64_t()> counter) {
    cycle_counter_ = std::move(counter);
  }

  void log(LogLevel level, LogCategory cat, std::string_view msg,
           std::string_view file = "", int line = 0,
           std::string_view func = "");

  template <typename... Args>
  void trace(LogCategory cat, std::string_view fmt, Args &&...args) {
    log(LogLevel::Trace, cat, format(fmt, std::forward<Args>(args)...));
  }

  template <typename... Args>
  void debug(LogCategory cat, std::string_view fmt, Args &&...args) {
    log(LogLevel::Debug, cat, format(fmt, std::forward<Args>(args)...));
  }

  template <typename... Args>
  void info(LogCategory cat, std::string_view fmt, Args &&...args) {
    log(LogLevel::Info, cat, format(fmt, std::forward<Args>(args)...));
  }

  template <typename... Args>
  void warn(LogCategory cat, std::string_view fmt, Args &&...args) {
    log(LogLevel::Warn, cat, format(fmt, std::forward<Args>(args)...));
  }

  template <typename... Args>
  void error(LogCategory cat, std::string_view fmt, Args &&...args) {
    log(LogLevel::Error, cat, format(fmt, std::forward<Args>(args)...));
  }

  template <typename... Args>
  void fatal(LogCategory cat, std::string_view fmt, Args &&...args) {
    log(LogLevel::Fatal, cat, format(fmt, std::forward<Args>(args)...));
  }

  template <typename... Args>
  static std::string format(std::string_view fmt, Args &&...args) {
    char buffer[1024];
    int len = std::snprintf(buffer, sizeof(buffer), fmt.data(),
                            std::forward<Args>(args)...);
    return std::string(buffer, len > 0 ? len : 0);
  }

private:
  Logger() = default;
  ~Logger() {
    if (file_.is_open())
      file_.close();
  }

  LogLevel min_level_ = LogLevel::Info;
  LogCategory categories_ = LogCategory::All;
  LogCallback callback_;
  std::ofstream file_;
  std::function<uint64_t()> cycle_counter_;
  std::mutex mutex_;
};

// Macro for easy logging with file/line/function
#define O2E_LOG(logger, level, cat, fmt, ...)                                  \
  logger.log(o2emu::LogLevel::level, cat,                                      \
             o2emu::Logger::format(fmt, ##__VA_ARGS__), __FILE__, __LINE__,    \
             __func__)

#define O2E_TRACE(cat, fmt, ...)                                               \
  O2E_LOG(Logger::instance(), Trace, cat, fmt, ##__VA_ARGS__)
#define O2E_DEBUG(cat, fmt, ...)                                               \
  O2E_LOG(Logger::instance(), Debug, cat, fmt, ##__VA_ARGS__)
#define O2E_INFO(cat, fmt, ...)                                                \
  O2E_LOG(Logger::instance(), Info, cat, fmt, ##__VA_ARGS__)
#define O2E_WARN(cat, fmt, ...)                                                \
  O2E_LOG(Logger::instance(), Warn, cat, fmt, ##__VA_ARGS__)
#define O2E_ERROR(cat, fmt, ...)                                               \
  O2E_LOG(Logger::instance(), Error, cat, fmt, ##__VA_ARGS__)
#define O2E_FATAL(cat, fmt, ...)                                               \
  O2E_LOG(Logger::instance(), Fatal, cat, fmt, ##__VA_ARGS__)

// Register access logging
#define O2E_LOG_READ(dev, reg, val)                                            \
  O2E_TRACE(LogCategory::dev, "READ  {} = 0x{:08x}", #reg, val)

#define O2E_LOG_WRITE(dev, reg, val)                                           \
  O2E_TRACE(LogCategory::dev, "WRITE {} = 0x{:08x}", #reg, val)

} // namespace o2emu