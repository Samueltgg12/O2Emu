/**
 * @file prom_loader.cpp
 * @brief PROM loading and execution
 */

#include <o2emu/cpu/cpu.h>
#include <o2emu/firmware/prom.h>
#include <o2emu/firmware/prom_loader.h>
#include <o2emu/logging/logger.h>
#include <o2emu/system/bus.h>

namespace o2emu::firmware {

PROMLoader::PROMLoader(system::Bus *bus, cpu::CPU *cpu)
    : bus_(bus), cpu_(cpu), prom_(bus, cpu) {}

bool PROMLoader::load_prom(const std::string &prom_path) {
  return prom_.load_image(prom_path);
}

void PROMLoader::execute_bootstrap() {
  // The PROM starts executing at the reset vector (0xBFC00000)
  // The CPU should already be reset to this address
  O2EMU_LOG_INFO_F("Starting PROM bootstrap at 0xBFC00000");

  // Run POST
  run_post();

  // Execute the PROM (sets up CPU and starts execution)
  prom_.execute();

  // The PROM will then initialize hardware and boot the OS
  // This is handled by the emulation loop
}

void PROMLoader::run_post() {
  O2EMU_LOG_INFO_F("Running POST sequence...");
  // POST is part of the PROM code, it will execute automatically
  // when the CPU runs from the reset vector
}

bool PROMLoader::boot_kernel(const std::string &kernel_path,
                             const std::string &initrd_path,
                             const std::string &cmdline) {
  O2EMU_LOG_INFO_F("Booting kernel: %s", kernel_path.c_str());
  // This would load an OS kernel (IRIX/Linux/NetBSD)
  // For now, just return false as not implemented
  (void)kernel_path;
  (void)initrd_path;
  (void)cmdline;
  return false;
}

void PROMLoader::map_prom_sections() {
  // The PROM class already maps sections in load_image()
  // This is kept for API compatibility
  if (!prom_.is_loaded()) {
    O2EMU_LOG_ERROR_F("Cannot map sections: PROM not loaded");
    return;
  }
  O2EMU_LOG_INFO_F("PROM sections already mapped by PROM::load_image()");
}

void PROMLoader::init_cpu_for_prom() {
  // The PROM::execute() method already initializes the CPU
  // This is kept for API compatibility
  O2EMU_LOG_INFO_F("CPU initialization handled by PROM::execute()");
  prom_.execute();
}

void PROMLoader::setup_environment() {
  // Set up PROM environment variables in memory
  // This would normally be done by the PROM code itself
  O2EMU_LOG_INFO_F("Setting up PROM environment...");
}

void PROMLoader::copy_sections() {
  // Already done in PROM::load_image()
  map_prom_sections();
}

} // namespace o2emu::firmware