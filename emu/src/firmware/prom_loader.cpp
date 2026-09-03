/**
 * @file prom_loader.cpp
 * @brief PROM image loader (SHDR format with embedded ELF)
 */

#include <algorithm>
#include <cstring>
#include <fstream>
#include <o2emu/firmware/prom_loader.h>
#include <o2emu/logging/logger.h>

namespace o2emu::firmware {

PROMImage::PROMImage()
    : data_(nullptr), size_(0), entry_point_(0), valid_(false) {}

PROMImage::~PROMImage() { delete[] data_; }

bool PROMImage::load(const std::string &path) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file) {
    O2EMU_LOG_ERROR("Failed to open PROM image: " << path);
    return false;
  }

  size_ = file.tellg();
  file.seekg(0, std::ios::beg);

  if (size_ == 0) {
    O2EMU_LOG_ERROR("PROM image is empty: " << path);
    return false;
  }

  data_ = new u8[size_];
  file.read(reinterpret_cast<char *>(data_), size_);
  file.close();

  // Verify checksum
  if (!verify_checksum()) {
    O2EMU_LOG_ERROR("PROM checksum verification failed");
    delete[] data_;
    data_ = nullptr;
    size_ = 0;
    return false;
  }

  // Parse SHDR sections
  if (!parse_shdr()) {
    O2EMU_LOG_ERROR("Failed to parse SHDR sections");
    delete[] data_;
    data_ = nullptr;
    size_ = 0;
    return false;
  }

  valid_ = true;
  O2EMU_LOG_INFO("Loaded PROM image: " << path << " (" << size_ << " bytes)");
  return true;
}

bool PROMImage::verify_checksum() const {
  // IP32 PROM uses two's complement checksum over entire image
  u32 sum = 0;
  for (u32 i = 0; i < size_; ++i) {
    sum += data_[i];
  }

  // Checksum should be 0 (two's complement)
  return (sum & 0xFF) == 0;
}

bool PROMImage::parse_shdr() {
  // SHDR format: 5 sections
  // Section 0: Header (0x0000-0x00FF)
  // Section 1: SLOADER (bootloader)
  // Section 2: POST1 (power-on self test)
  // Section 3: Firmware (main PROM)
  // Section 4: Trailing data

  if (size_ < 256) {
    return false;
  }

  // Check for SHDR magic
  if (data_[0] != 'S' || data_[1] != 'H' || data_[2] != 'D' ||
      data_[3] != 'R') {
    O2EMU_LOG_WARN("No SHDR magic found, assuming raw image");
    // Try to find ELF header
    return find_elf();
  }

  // Parse section headers (simplified)
  // Each section header is 32 bytes
  u32 offset = 0x100; // After main header

  for (int i = 0; i < 5; ++i) {
    if (offset + 32 > size_)
      break;

    Section section;
    section.name = std::string(reinterpret_cast<char *>(data_ + offset), 16);
    section.offset = *reinterpret_cast<u32 *>(data_ + offset + 16);
    section.size = *reinterpret_cast<u32 *>(data_ + offset + 20);
    section.load_addr = *reinterpret_cast<u32 *>(data_ + offset + 24);
    section.flags = *reinterpret_cast<u32 *>(data_ + offset + 28);

    sections_.push_back(section);
    offset += 32;

    O2EMU_LOG_DEBUG("SHDR section " << i << ": " << section.name << " offset=0x"
                                    << std::hex << section.offset << " size=0x"
                                    << section.size << " load=0x"
                                    << section.load_addr << std::dec);
  }

  // Find entry point from firmware section
  for (const auto &section : sections_) {
    if (section.name.find("FIRMWARE") != std::string::npos ||
        section.name.find("firmware") != std::string::npos) {
      entry_point_ = section.load_addr;
      break;
    }
  }

  return true;
}

bool PROMImage::find_elf() {
  // Search for ELF magic (0x7F 'E' 'L' 'F')
  for (u32 i = 0; i < size_ - 4; ++i) {
    if (data_[i] == 0x7F && data_[i + 1] == 'E' && data_[i + 2] == 'L' &&
        data_[i + 3] == 'F') {
      O2EMU_LOG_INFO("Found ELF header at offset 0x" << std::hex << i
                                                     << std::dec);

      // Parse ELF header to find entry point
      if (i + 52 <= size_) {
        u32 entry = *reinterpret_cast<u32 *>(data_ + i + 24);
        entry_point_ = entry;
        O2EMU_LOG_INFO("ELF entry point: 0x" << std::hex << entry_point_
                                             << std::dec);
      }
      return true;
    }
  }
  return false;
}

const u8 *PROMImage::data() const { return data_; }

u32 PROMImage::size() const { return size_; }

u32 PROMImage::entry_point() const { return entry_point_; }

bool PROMImage::valid() const { return valid_; }

const std::vector<PROMImage::Section> &PROMImage::sections() const {
  return sections_;
}

// PROMLoader implementation
PROMLoader::PROMLoader(cpu::ICpu &cpu, memory::Memory &memory)
    : cpu_(cpu), memory_(memory), prom_() {}

bool PROMLoader::load_prom(const std::string &prom_path) {
  return prom_.load(prom_path);
}

void PROMLoader::execute_bootstrap() {
  // The PROM starts executing at the reset vector (0xBFC00000)
  // The CPU should already be reset to this address
  O2EMU_LOG_INFO("Starting PROM bootstrap at 0xBFC00000");

  // Run POST
  run_post();

  // The PROM will then initialize hardware and boot the OS
  // This is handled by the emulation loop
}

void PROMLoader::run_post() {
  O2EMU_LOG_INFO("Running POST sequence...");
  // POST is part of the PROM code, it will execute automatically
  // when the CPU runs from the reset vector
}

bool PROMLoader::boot_kernel(const std::string &kernel_path,
                             const std::string &initrd_path,
                             const std::string &cmdline) {
  O2EMU_LOG_INFO("Booting kernel: " << kernel_path);
  // This would load an OS kernel (IRIX/Linux/NetBSD)
  // For now, just return false as not implemented
  (void)kernel_path;
  (void)initrd_path;
  (void)cmdline;
  return false;
}

void PROMLoader::map_prom_sections() {
  if (!prom_.valid()) {
    O2EMU_LOG_ERROR("Cannot map sections: PROM not loaded");
    return;
  }

  O2EMU_LOG_INFO("Mapping PROM sections to memory...");

  for (const auto &section : prom_.sections()) {
    if (section.size == 0)
      continue;

    const u8 *src = prom_.data() + section.offset;
    u32 dst = section.load_addr;

    O2EMU_LOG_DEBUG("Mapping section " << section.name << " to 0x" << std::hex
                                       << dst << " size 0x" << section.size
                                       << std::dec);

    // Copy section data to memory
    for (u32 i = 0; i < section.size; ++i) {
      memory_.write8(dst + i, src[i]);
    }
  }
}

void PROMLoader::init_cpu_for_prom() {
  O2EMU_LOG_INFO("Initializing CPU for PROM execution...");

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

  O2EMU_LOG_INFO("CPU initialized for PROM: PC=0x" << std::hex << cpu_.pc()
                                                   << std::dec);
}

void PROMLoader::setup_environment() {
  // Set up PROM environment variables in memory
  // This would normally be done by the PROM code itself
  O2EMU_LOG_INFO("Setting up PROM environment...");
}

void PROMLoader::copy_sections() {
  // Already done in map_prom_sections
  map_prom_sections();
}

} // namespace o2emu::firmware