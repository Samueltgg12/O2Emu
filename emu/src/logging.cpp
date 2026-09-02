#include "o2emu/logging.h"
#include <chrono>
#include <iomanip>
#include <sstream>

namespace o2emu {

void Logger::set_output_file(const std::string &path) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (file_.is_open())
    file_.close();
  file_.open(path, std::ios::out | std::ios::app);
}

void Logger::log(LogLevel level, LogCategory cat, std::string_view msg,
                 std::string_view file, int line, std::string_view func) {
  if (level < min_level_)
    return;
  if ((static_cast<uint16_t>(categories_) & static_cast<uint16_t>(cat)) == 0)
    return;

  LogEntry entry;
  entry.level = level;
  entry.category = cat;
  entry.timestamp = std::chrono::steady_clock::now();
  entry.cycle_count = cycle_counter_ ? cycle_counter_() : 0;
  entry.message = msg;
  entry.file = file;
  entry.line = line;
  entry.function = func;

  // Call callback if set
  if (callback_) {
    callback_(entry);
  }

  // Write to file if open
  if (file_.is_open()) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) %
              1000;

    file_ << std::put_time(std::localtime(&time_t), "%H:%M:%S") << '.'
          << std::setfill('0') << std::setw(3) << ms.count() << " ["
          << static_cast<int>(level) << "]"
          << " [" << static_cast<int>(cat) << "]"
          << " " << msg << "\n";
    file_.flush();
  }

  // Also write to stderr for errors and above
  if (level >= LogLevel::Error) {
    std::cerr << "[" << static_cast<int>(level) << "] " << msg << "\n";
  }
}

} // namespace o2emu