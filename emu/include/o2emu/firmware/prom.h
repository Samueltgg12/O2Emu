#pragma once

/**
 * @file prom.h
 * @brief IP32 PROM firmware structures and loading
 *
 * Based on decompiled PROM (samples/decompiled-prom/) and IRIX stand/arcs/
 */

#include <cstdint>
#include <o2emu/o2emu.h>
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

class PROM {
public:
  PROM();
  ~PROM() = default;

  // Load PROM from file
  bool load_from_file(const std::string &path);

  // Load PROM from memory buffer
  bool load_from_buffer(const u8 *data, size_t size);

  // Get PROM entry point
  u32 entry_point() const { return entry_point_; }

  // Get section data
  const u8 *section_data(PromSection sect) const;
  u32 section_size(PromSection sect) const;
  u32 section_load_addr(PromSection sect) const;

  // Verify checksums
  bool verify_checksums() const;

  // Get raw PROM image
  const std::vector<u8> &raw_image() const { return image_; }

  // PROM environment variables (from decompiled env.S)
  struct EnvVar {
    std::string name;
    std::string value;
  };
  const std::vector<EnvVar> &environment() const { return env_vars_; }

  // Reset
  void reset();

private:
  std::vector<u8> image_;
  SHDRHeader shdr_ = {};
  std::vector<SHDRSection> sections_;
  ELFHeader elf_ = {};
  u32 entry_point_ = 0;
  std::vector<EnvVar> env_vars_;

  // Parse SHDR header
  bool parse_shdr();

  // Parse ELF header (embedded in section 4)
  bool parse_elf();

  // Parse environment variables
  bool parse_environment();

  // Compute two's complement checksum
  static u32 compute_checksum(const u8 *data, size_t size);
};

} // namespace o2emu::firmware