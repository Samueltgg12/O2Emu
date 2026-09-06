/**
 * @file cpu_factory.cpp
 * @brief CPU factory implementation
 */

#include <memory>
#include <o2emu/cpu/cp0.h>
#include <o2emu/cpu/cpu.h>
#include <o2emu/cpu/cpu_interface.h>
#include <o2emu/cpu/mips_r10000.h>
#include <o2emu/cpu/mips_r5000.h>
#include <o2emu/system/bus.h>

namespace o2emu::cpu {

// Adapter for the base CPU class - inherits from CPU and implements ICpu
// interface
class CpuAdapter : public CPU {
public:
  CpuAdapter() = default;

  // ICpu interface methods not in CPU
  using ReadCallback = ICpu::ReadCallback;
  using WriteCallback = ICpu::WriteCallback;

  uint32_t gpr(int reg) const { return state().gpr[reg]; }

  void set_gpr(int reg, uint32_t value) { state().gpr[reg] = value; }

  uint64_t gpr64(int reg) const { return state().gpr[reg]; }

  void set_gpr64(int reg, uint64_t value) {
    state().gpr[reg] = static_cast<uint32_t>(value);
  }

  uint32_t pc() const { return state().pc; }

  void set_pc(uint32_t pc) { state().pc = pc; }

  uint32_t cp0_reg(CP0::Register reg) const { return cp0().read(reg); }

  void set_cp0_reg(CP0::Register reg, uint32_t value) {
    cp0().write(reg, value);
  }

  uint64_t cycles() const { return cycles_executed(); }

  uint64_t instructions() const { return cycles_executed(); }

  uint64_t cycles_executed() const { return CPU::cycles_executed(); }

  CPUType type() const { return CPUType::R5000; }

  const char *type_name() const { return "MIPS R5000 (base)"; }

  // ICpu state() - non-const version
  CPUState &state() { return CPU::state(); }

  // ICpu state() - const version
  const CPUState &state() const { return CPU::state(); }
};

// Adapter for MIPSR5000 - uses composition over inheritance
class MIPSR5000Adapter : public CPU {
public:
  explicit MIPSR5000Adapter(system::Bus *bus) : mips_(bus) {}

  void reset(uint32_t reset_vector = 0xBFC00000) {
    mips_.reset();
    mips_.set_pc(reset_vector);
  }

  void step() { mips_.tick(1); }

  void run(uint64_t cycles) { mips_.tick(cycles); }

  void run_until(uint32_t target_pc) {
    while (mips_.pc() != target_pc) {
      mips_.tick(1);
    }
  }

  void set_memory_read_callback(ReadCallback cb) {
    // MIPSR5000 uses Bus interface, not callbacks
    (void)cb;
  }

  void set_memory_write_callback(WriteCallback cb) { (void)cb; }

  void raise_interrupt(InterruptLine line) {
    // Not directly supported in MIPSR5000
    (void)line;
  }

  void clear_interrupt(InterruptLine line) { (void)line; }

  uint32_t gpr(int reg) const { return mips_.gpr(reg); }

  void set_gpr(int reg, uint32_t value) { mips_.set_gpr(reg, value); }

  uint64_t gpr64(int reg) const { return mips_.gpr64(reg); }

  void set_gpr64(int reg, uint64_t value) { mips_.set_gpr64(reg, value); }

  uint32_t pc() const { return mips_.pc(); }

  void set_pc(uint32_t pc) { mips_.set_pc(pc); }

  uint32_t cp0_reg(CP0::Register reg) const { return mips_.cp0_reg(reg); }

  void set_cp0_reg(CP0::Register reg, uint32_t value) {
    mips_.set_cp0_reg(reg, value);
  }

  uint64_t cycles() const { return mips_.cycles(); }

  uint64_t instructions() const { return mips_.instructions(); }

  uint64_t cycles_executed() const { return mips_.cycles(); }

  CPUState &state() {
    cached_state_.pc = mips_.pc();
    cached_state_.hi = mips_.hi();
    cached_state_.lo = mips_.lo();
    cached_state_.fcr0 = mips_.fcr0();
    cached_state_.fcr31 = mips_.fcr31();
    cached_state_.llbit = mips_.llbit();
    for (int i = 0; i < 32; ++i) {
      cached_state_.gpr[i] = mips_.gpr(i);
      cached_state_.fpr[i] = mips_.fpr(i);
    }
    cached_state_.cp0 = nullptr;
    return cached_state_;
  }

  const CPUState &state() const {
    static thread_local CPUState cached;
    cached = const_cast<MIPSR5000Adapter *>(this)->state();
    return cached;
  }

  void dump_registers() const { mips_.dump_registers(); }

  void disassemble(uint32_t addr, char *buffer, size_t size) const {
    // Not implemented
    (void)addr;
    (void)buffer;
    (void)size;
  }

  CPUType type() const { return CPUType::R5000; }

  const char *type_name() const { return "MIPS R5000"; }

private:
  MIPSR5000 mips_;
  CPUState cached_state_;
};

// Adapter for MIPSR10000 - uses composition over inheritance
class MIPSR10000Adapter : public CPU {
public:
  explicit MIPSR10000Adapter(system::Bus *bus, MIPSR10000::Variant variant =
                                                   MIPSR10000::Variant::R10000)
      : mips_(bus, variant) {}

  void reset(uint32_t reset_vector = 0xBFC00000) {
    mips_.reset();
    mips_.set_pc(reset_vector);
  }

  void step() { mips_.tick(1); }

  void run(uint64_t cycles) { mips_.tick(cycles); }

  void run_until(uint32_t target_pc) {
    while (mips_.pc() != target_pc) {
      mips_.tick(1);
    }
  }

  void set_memory_read_callback(ReadCallback cb) { (void)cb; }

  void set_memory_write_callback(WriteCallback cb) { (void)cb; }

  void raise_interrupt(InterruptLine line) { (void)line; }

  void clear_interrupt(InterruptLine line) { (void)line; }

  uint32_t gpr(int reg) const { return mips_.gpr(reg); }

  void set_gpr(int reg, uint32_t value) { mips_.set_gpr(reg, value); }

  uint64_t gpr64(int reg) const { return mips_.gpr64(reg); }

  void set_gpr64(int reg, uint64_t value) { mips_.set_gpr64(reg, value); }

  uint32_t pc() const { return mips_.pc(); }

  void set_pc(uint32_t pc) { mips_.set_pc(pc); }

  uint32_t cp0_reg(CP0::Register reg) const { return mips_.cp0_reg(reg); }

  void set_cp0_reg(CP0::Register reg, uint32_t value) {
    mips_.set_cp0_reg(reg, value);
  }

  uint64_t cycles() const { return mips_.cycles(); }

  uint64_t instructions() const { return mips_.instructions(); }

  uint64_t cycles_executed() const { return mips_.cycles(); }

  CPUState &state() {
    cached_state_.pc = mips_.pc();
    cached_state_.hi = mips_.hi();
    cached_state_.lo = mips_.lo();
    cached_state_.fcr0 = mips_.fcr0();
    cached_state_.fcr31 = mips_.fcr31();
    cached_state_.llbit = mips_.llbit();
    for (int i = 0; i < 32; ++i) {
      cached_state_.gpr[i] = mips_.gpr(i);
      cached_state_.fpr[i] = mips_.fpr(i);
    }
    cached_state_.cp0 = nullptr;
    return cached_state_;
  }

  const CPUState &state() const {
    static thread_local CPUState cached;
    cached = const_cast<MIPSR10000Adapter *>(this)->state();
    return cached;
  }

  void dump_registers() const { mips_.dump_registers(); }

  void disassemble(uint32_t addr, char *buffer, size_t size) const {
    (void)addr;
    (void)buffer;
    (void)size;
  }

  CPUType type() const {
    return mips_.variant() == MIPSR10000::Variant::R12000 ? CPUType::R12000
                                                          : CPUType::R10000;
  }

  const char *type_name() const {
    return mips_.variant() == MIPSR10000::Variant::R12000 ? "MIPS R12000"
                                                          : "MIPS R10000";
  }

private:
  MIPSR10000 mips_;
  CPUState cached_state_;
};

std::unique_ptr<CPU> create_cpu(CPUType type, system::Bus *bus) {
  switch (type) {
  case CPUType::R5000:
    if (bus) {
      return std::make_unique<MIPSR5000Adapter>(bus);
    } else {
      return std::make_unique<CpuAdapter>();
    }
  case CPUType::R10000:
    if (bus) {
      return std::make_unique<MIPSR10000Adapter>(bus,
                                                 MIPSR10000::Variant::R10000);
    } else {
      return std::make_unique<CpuAdapter>();
    }
  case CPUType::R12000:
    if (bus) {
      return std::make_unique<MIPSR10000Adapter>(bus,
                                                 MIPSR10000::Variant::R12000);
    } else {
      return std::make_unique<CpuAdapter>();
    }
  default:
    return std::make_unique<CpuAdapter>();
  }
}

} // namespace o2emu::cpu