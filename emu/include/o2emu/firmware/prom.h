#pragma once

/**
 * @file prom.h
 * @brief IP32 PROM firmware structures, loading, and execution
 *
 * Based on decompiled PROM (samples/decompiled-prom/) and IRIX stand/arcs/
 */

#include <cstdint>
#include <memory>
#include <o2emu/cpu/cpu.h>
#include <o2emu/o2emu.h>
#include <o2emu/system/bus.h>
#include <string>
#include <vector>

namespace o2emu::firmware {

// PROM image format (SHDR - SGI Header)
#pragma pack(push, 1)
struct SHDRHeader {
  u32 magic;          // 0x48445253 "SHDR"
  u32 version;        // Header version
  u32 num_sections;   // Number of sections (typically 5)
  u32 section_offset; // Offset to section table
  u32 checksum;       // Two's complement checksum
  u32 reserved[3];
};

struct SHDRSection {
  u32 type;        // Section type
  u32 flags;       // Section flags
  u32 load_addr;   // Load address (VMA)
  u32 file_offset; // Offset in file
  u32 size;        // Size in file
  u32 mem_size;    // Size in memory (may be larger for BSS)
  u32 checksum;    // Section checksum
  u32 align;       // Alignment requirement
};

struct ELFHeader {
  u8 ident[16];  // ELF identification
  u16 type;      // Object file type
  u16 machine;   // Architecture (MIPS = 8)
  u32 version;   // Object file version
  u32 entry;     // Entry point
  u32 phoff;     // Program header offset
  u32 shoff;     // Section header offset
  u32 flags;     // Processor-specific flags
  u16 ehsize;    // ELF header size
  u16 phentsize; // Program header entry size
  u16 phnum;     // Program header count
  u16 shentsize; // Section header entry size
  u16 shnum;     // Section header count
  u16 shstrndx;  // Section header string table index
};
#pragma pack(pop)

// Section types
enum SectionType : uint32_t {
  SECT_TEXT = 0x01,
  SECT_DATA = 0x02,
  SECT_BSS = 0x03,
  SECT_ELF = 0x04, // Embedded ELF header
  SECT_CHECKSUM = 0x05,
};

// PROM sections (from decompiled PROM)
enum PromSection : uint32_t {
  PROM_SECT_BOOTSTRAP = 0, // Initial bootstrap code
  PROM_SECT_POST1 = 1,     // POST1 - memory sizing, basic init
  PROM_SECT_SLOADER = 2,   // Secondary loader
  PROM_SECT_ENV = 3,       // Environment variables
  PROM_SECT_ELF = 4,       // Embedded ELF (main PROM)
};

// PROM image container (for execution)
class PROMImage {
public:
  PROMImage() = default;
  ~PROMImage() = default;

  bool load(const std::string &path);
  bool load_from_buffer(const u8 *data, size_t size);

  const u8 *data() const { return image_.data(); }
  u32 size() const { return static_cast<u32>(image_.size()); }
  u32 entry_point() const { return entry_point_; }
  bool valid() const { return !image_.empty(); }

private:
  std::vector<u8> image_;
  u32 entry_point_ = 0;

  bool parse_shdr();
  bool parse_elf();
  static u32 compute_checksum(const u8 *data, size_t size);
};

class PROM {
public:
  PROM(o2emu::Bus *bus, o2emu::CPU *cpu);
  ~PROM() = default;

  // Load PROM image from file
  bool load_image(const std::string &path);

  // Map PROM image to memory
  void map_to_memory();

  // Execute PROM (set CPU to entry point)
  void execute();

  // Reset PROM (re-map and restart)
  void reset();

  // Check if PROM is loaded
  bool is_loaded() const { return loaded_; }

  // Get PROM entry point
  u32 entry_point() const;

  // Get PROM image
  const PROMImage *image() const { return image_.get(); }

private:
  o2emu::Bus *bus_ = nullptr;
  o2emu::CPU *cpu_ = nullptr;
  std::unique_ptr<PROMImage> image_;
  bool loaded_ = false;
};

} // namespace firmware
} // namespace o2emu