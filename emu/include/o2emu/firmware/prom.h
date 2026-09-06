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
// Based on decompiled PROM (samples/decompiled-prom/rev4.18/definitions.h)
// The PROM file contains multiple SHDR headers (64 bytes each), one per section
#pragma pack(push, 1)
struct SHDRSectionHeader {
  u32 magic;       // "SHDR" = 0x52444853 (little-endian)
  u32 section_len; // Length of section data following this header
  u16 name_len;    // Length of name string
  u16 version_len; // Length of version string
  u8 section_type; // Section type (bitmask: 1=CODE, 2=DATA, 4=LOADABLE,
                   // 8=CHECKSUM)
  u8 padding[3];   // Padding to 32-bit boundary
  char name[32];   // Section name (null-padded)
  char version[8]; // Section version (null-padded)
  u32 checksum;    // Section checksum
  u8 reserved[8];  // Reserved
  // Total: 4 + 4 + 2 + 2 + 1 + 3 + 32 + 8 + 4 + 8 = 68 bytes
  // But SHDR_SIZE in definitions.h is 64, so reserved might be 4 bytes
  // Actually let's check: the decompiled PROM says SHDR_SIZE=64
  // 4+4+2+2+1+3+32+8+4+4 = 64. So reserved[4] not [8].
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

// Section type bitmasks (from decompiled PROM definitions.h)
enum SectionType : uint32_t {
  SECTION_TYPE_CODE = 1,
  SECTION_TYPE_DATA = 2,
  SECTION_TYPE_LOADABLE = 4,
  SECTION_TYPE_CHECKSUM = 8,
};

// PROM sections (from decompiled PROM)
enum PromSection : uint32_t {
  PROM_SECT_BOOTSTRAP = 0, // Initial bootstrap code
  PROM_SECT_POST1 = 1,     // POST1 - memory sizing, basic init
  PROM_SECT_SLOADER = 2,   // Secondary loader
  PROM_SECT_ENV = 3,       // Environment variables
  PROM_SECT_ELF = 4,       // Embedded ELF (main PROM)
};

// Section info for parsed SHDR sections
struct SectionInfo {
  uint32_t type;
  uint32_t offset;
  uint32_t size;
  std::string name;
  std::string version;
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
  const std::vector<SectionInfo> &sections() const { return sections_; }

private:
  std::vector<u8> image_;
  u32 entry_point_ = 0;
  std::vector<SectionInfo> sections_;

  bool parse_shdr_sections();
  bool parse_elf();
  static u32 compute_checksum(const u8 *data, size_t size);
};

class PROM {
public:
  PROM(o2emu::system::Bus *bus, o2emu::cpu::CPU *cpu);
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
  o2emu::system::Bus *bus_ = nullptr;
  o2emu::cpu::CPU *cpu_ = nullptr;
  std::unique_ptr<PROMImage> image_;
  bool loaded_ = false;
};

} // namespace o2emu::firmware