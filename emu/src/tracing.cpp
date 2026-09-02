#include "o2emu/tracing.h"
#include <iomanip>
#include <sstream>

namespace o2emu {

void Tracer::set_instruction_trace_file(const std::string &path) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (instr_file_.is_open())
    instr_file_.close();
  instr_file_.open(path, std::ios::out | std::ios::trunc);
  instr_file_
      << "cycle,pc,instruction,gpr0-gpr31,hi,lo,cp0_status,cp0_cause,cp0_epc,"
         "cp0_badvaddr,branch_taken,branch_target,disassembly\n";
}

void Tracer::set_memory_trace_file(const std::string &path) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (mem_file_.is_open())
    mem_file_.close();
  mem_file_.open(path, std::ios::out | std::ios::trunc);
  mem_file_ << "cycle,addr,size,write,value,pc,device\n";
}

void Tracer::set_register_trace_file(const std::string &path) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (reg_file_.is_open())
    reg_file_.close();
  reg_file_.open(path, std::ios::out | std::ios::trunc);
  reg_file_ << "cycle,device,offset,size,write,value,pc\n";
}

void Tracer::trace_instruction(const TraceEntry &entry) {
  if (!trace_instructions_)
    return;

  std::lock_guard<std::mutex> lock(mutex_);
  instr_trace_.push_back(entry);
  flush_if_needed(instr_trace_, instr_file_);
}

void Tracer::trace_memory_access(const MemTraceEntry &entry) {
  if (!trace_memory_)
    return;

  std::lock_guard<std::mutex> lock(mutex_);
  mem_trace_.push_back(entry);
  flush_if_needed(mem_trace_, mem_file_);
}

void Tracer::trace_register_access(const RegTraceEntry &entry) {
  if (!trace_registers_)
    return;

  std::lock_guard<std::mutex> lock(mutex_);
  reg_trace_.push_back(entry);
  flush_if_needed(reg_trace_, reg_file_);
}

void Tracer::flush() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (instr_file_.is_open()) {
    for (const auto &e : instr_trace_) {
      instr_file_ << e.cycle << "," << std::hex << e.pc << "," << e.instruction
                  << ",";
      for (int i = 0; i < 32; ++i) {
        instr_file_ << std::hex << e.gpr[i] << (i < 31 ? "," : "");
      }
      instr_file_ << "," << std::hex << e.hi << "," << e.lo << ","
                  << e.cp0_status << "," << e.cp0_cause << "," << e.cp0_epc
                  << "," << e.cp0_badvaddr << ","
                  << (e.branch_taken ? "1" : "0") << "," << std::hex
                  << e.branch_target << ","
                  << "\"" << e.disassembly << "\"\n";
    }
    instr_trace_.clear();
    instr_file_.flush();
  }
  if (mem_file_.is_open()) {
    for (const auto &e : mem_trace_) {
      mem_file_ << e.cycle << "," << std::hex << e.addr << "," << std::dec
                << e.size << "," << (e.write ? "1" : "0") << "," << std::hex
                << e.value << "," << e.pc << "," << e.device << "\n";
    }
    mem_trace_.clear();
    mem_file_.flush();
  }
  if (reg_file_.is_open()) {
    for (const auto &e : reg_trace_) {
      reg_file_ << e.cycle << "," << e.device << "," << std::hex << e.offset
                << "," << std::dec << e.size << "," << (e.write ? "1" : "0")
                << "," << std::hex << e.value << "," << e.pc << "\n";
    }
    reg_trace_.clear();
    reg_file_.flush();
  }
}

void Tracer::clear() {
  std::lock_guard<std::mutex> lock(mutex_);
  instr_trace_.clear();
  mem_trace_.clear();
  reg_trace_.clear();
}

void Tracer::flush_if_needed(std::vector<TraceEntry> &vec,
                             std::ofstream &file) {
  if (vec.size() >= flush_threshold_ && file.is_open()) {
    for (const auto &e : vec) {
      file << e.cycle << "," << std::hex << e.pc << "," << e.instruction << ",";
      for (int i = 0; i < 32; ++i) {
        file << std::hex << e.gpr[i] << (i < 31 ? "," : "");
      }
      file << "," << std::hex << e.hi << "," << e.lo << "," << e.cp0_status
           << "," << e.cp0_cause << "," << e.cp0_epc << "," << e.cp0_badvaddr
           << "," << (e.branch_taken ? "1" : "0") << "," << std::hex
           << e.branch_target << ","
           << "\"" << e.disassembly << "\"\n";
    }
    vec.clear();
    file.flush();
  }
}

void Tracer::flush_if_needed(std::vector<MemTraceEntry> &vec,
                             std::ofstream &file) {
  if (vec.size() >= flush_threshold_ && file.is_open()) {
    for (const auto &e : vec) {
      file << e.cycle << "," << std::hex << e.addr << "," << std::dec << e.size
           << "," << (e.write ? "1" : "0") << "," << std::hex << e.value << ","
           << e.pc << "," << e.device << "\n";
    }
    vec.clear();
    file.flush();
  }
}

void Tracer::flush_if_needed(std::vector<RegTraceEntry> &vec,
                             std::ofstream &file) {
  if (vec.size() >= flush_threshold_ && file.is_open()) {
    for (const auto &e : vec) {
      file << e.cycle << "," << e.device << "," << std::hex << e.offset << ","
           << std::dec << e.size << "," << (e.write ? "1" : "0") << ","
           << std::hex << e.value << "," << e.pc << "\n";
    }
    vec.clear();
    file.flush();
  }
}

TraceScope::TraceScope(const char *name, uint32_t pc)
    : name_(name), pc_(pc), start_cycle_(0) {
  // Could add function entry trace here
}

TraceScope::~TraceScope() {
  // Could add function exit trace here
}

} // namespace o2emu