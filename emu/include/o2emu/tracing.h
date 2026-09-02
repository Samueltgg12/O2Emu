#pragma once

#include <chrono>
#include <cstdint>
#include <fstream>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace o2emu {

// Instruction trace entry
struct TraceEntry {
  uint64_t cycle;
  uint32_t pc;
  uint32_t instruction;
  uint32_t gpr[32];
  uint32_t hi, lo;
  uint32_t cp0_status;
  uint32_t cp0_cause;
  uint32_t cp0_epc;
  uint32_t cp0_badvaddr;
  bool branch_taken;
  uint32_t branch_target;
  std::string disassembly;
};

// Memory access trace entry
struct MemTraceEntry {
  uint64_t cycle;
  uint32_t addr;
  uint32_t size; // 1, 2, 4, 8
  bool write;
  uint64_t value;
  uint32_t pc;
  std::string device; // "CRIME", "RE", "GBE", "MACE", "RAM", "PROM", etc.
};

// Register access trace entry
struct RegTraceEntry {
  uint64_t cycle;
  std::string device;
  uint32_t offset;
  uint32_t size;
  bool write;
  uint64_t value;
  uint32_t pc;
};

class Tracer {
public:
  static Tracer &instance() {
    static Tracer tracer;
    return tracer;
  }

  void enable_instruction_trace(bool enable) { trace_instructions_ = enable; }
  void enable_memory_trace(bool enable) { trace_memory_ = enable; }
  void enable_register_trace(bool enable) { trace_registers_ = enable; }

  void set_instruction_trace_file(const std::string &path);
  void set_memory_trace_file(const std::string &path);
  void set_register_trace_file(const std::string &path);

  void trace_instruction(const TraceEntry &entry);
  void trace_memory_access(const MemTraceEntry &entry);
  void trace_register_access(const RegTraceEntry &entry);

  void flush();
  void clear();

  // Get trace buffers for analysis
  const std::vector<TraceEntry> &instruction_trace() const {
    return instr_trace_;
  }
  const std::vector<MemTraceEntry> &memory_trace() const { return mem_trace_; }
  const std::vector<RegTraceEntry> &register_trace() const {
    return reg_trace_;
  }

private:
  Tracer() = default;
  ~Tracer() { flush(); }

  bool trace_instructions_ = false;
  bool trace_memory_ = false;
  bool trace_registers_ = false;

  std::vector<TraceEntry> instr_trace_;
  std::vector<MemTraceEntry> mem_trace_;
  std::vector<RegTraceEntry> reg_trace_;

  std::ofstream instr_file_;
  std::ofstream mem_file_;
  std::ofstream reg_file_;

  std::mutex mutex_;
  size_t flush_threshold_ = 10000;

  void flush_if_needed(std::vector<TraceEntry> &vec, std::ofstream &file);
  void flush_if_needed(std::vector<MemTraceEntry> &vec, std::ofstream &file);
  void flush_if_needed(std::vector<RegTraceEntry> &vec, std::ofstream &file);
};

// RAII trace scope for function tracing
class TraceScope {
public:
  TraceScope(const char *name, uint32_t pc = 0);
  ~TraceScope();

private:
  const char *name_;
  uint32_t pc_;
  uint64_t start_cycle_;
};

} // namespace o2emu