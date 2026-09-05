/**
 * @file prom_loader.cpp
 * @brief PROM loading and execution
 */

#include <o2emu/firmware/prom_loader.h>
#include <o2emu/logging/logger.h>

namespace o2emu::firmware {

PROMLoader::PROMLoader(cpu::ICpu &cpu, memory::Memory &memory)
    : cpu_(cpu), memory_(memory), prom_() {}

bool PROMLoader::load_prom(const std::string &prom_path) {
  return prom_.load(prom_path);
}

void PROMLoader::execute_bootstrap() {
  // The PROM starts executing at the reset vector (0xBFC00000)
  // The CPU should already be reset to this address
  O2EMU_LOG_INFO_F("Starting PROM bootstrap at 0xBFC00000");

  // Run POST
  run_post();

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
  if (!prom_.is_loaded()) {
    O2EMU_LOG_ERROR_F("Cannot map sections: PROM not loaded");
    return;
  }

  const PROMImage *image = prom_.image();
  if (!image || !image->valid()) {
    O2EMU_LOG_ERROR_F("Cannot map sections: PROM image not valid");
    return;
  }

  O2EMU_LOG_INFO_F("Mapping PROM sections to memory...");

  for (const auto &section : image->sections()) {
    if (section.size == 0)
      continue;

    const u8 *src = image->data() + section.offset;
    u32 dst = section.load_addr;

    O2EMU_LOG_DEBUG_F("Mapping section %s to 0x%08X size 0x%08X",
                      section.name.c_str(), dst, section.size);

    // Copy section data to memory
    for (u32 i = 0; i < section.size; ++i) {
      memory_.write8(dst + i, src[i]);
    }
  }
}

void PROMLoader::init_cpu_for_prom() {
  O2EMU_LOG_INFO_F("Initializing CPU for PROM execution...");

  // Set CPU to PROM reset vector
  cpu_.set_pc(0xBFC00000);

  // Initialize CP0 registers for PROM
  // Status register: BEV=1 (bootstrap exception vectors), KU=0 (kernel mode)
  cpu_.set_cp0_reg(12, 0x00400004); // Status

  // Cause register: clear
  cpu_.set_cp0_reg(13, 0);

  // EPC: reset vector
  cpu_.set_cp0_reg(14, 0xBFC00000);

  // Config register
  cpu_.set_cp0_reg(16, 0x0006E463); // MIPS IV, 64-bit FPU, etc.

  // Set initial stack pointer in kseg0
  cpu_.set_gpr(29, 0x80000000); // SP
  cpu_.set_gpr(28, 0x80000000); // GP

  // Clear other registers
  for (int i = 1; i < 28; ++i) {
    cpu_.set_gpr(i, 0);
  }
  cpu_.set_gpr(0, 0); // $zero

  O2EMU_LOG_INFO_F("CPU initialized for PROM: PC=0x%08X", cpu_.pc());
}

void PROMLoader::setup_environment() {
  // Set up PROM environment variables in memory
  // This would normally be done by the PROM code itself
  O2EMU_LOG_INFO_F("Setting up PROM environment...");
}

void PROMLoader::copy_sections() {
  // Already done in map_prom_sections
  map_prom_sections();
}

} // namespace o2emu::firmware