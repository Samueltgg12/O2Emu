#pragma once

/**
 * @file prom_loader.h
 * @brief PROM loading and execution
 */

#include <memory>
#include <o2emu/firmware/prom.h>
#include <o2emu/o2emu.h>

namespace o2emu::cpu {
class CPU;
}

namespace o2emu::memory {
class Memory;
}

namespace o2emu::firmware {

class PROMLoader {
public:
  PROMLoader(cpu::CPU &cpu, memory::Memory &memory);
  ~PROMLoader() = default;

  // Load and prepare PROM for execution
  bool load_prom(const std::string &prom_path);

  // Execute PROM bootstrap (starts at reset vector 0xBFC00000)
  void execute_bootstrap();

  // Run POST sequence
  void run_post();

  // Load and jump to OS kernel (IRIX/Linux/NetBSD)
  bool boot_kernel(const std::string &kernel_path,
                   const std::string &initrd_path = "",
                   const std::string &cmdline = "");

  // Get loaded PROM
  const PROM &prom() const { return prom_; }
  PROM &prom() { return prom_; }

private:
  cpu::CPU &cpu_;
  memory::Memory &memory_;
  PROM prom_;

  // Map PROM sections into memory
  void map_prom_sections();

  // Initialize CPU state for PROM execution
  void init_cpu_for_prom();

  // Set up PROM environment variables
  void setup_environment();

  // Copy PROM sections to their load addresses
  void copy_sections();
};

} // namespace o2emu::firmware