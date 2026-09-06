/**
 * @file prom.cpp
 * @brief PROM firmware execution
 */

#include <cstdio>
#include <cstring>
#include <o2emu/cpu/cp0.h>
#include <o2emu/cpu/cpu.h>
#include <o2emu/firmware/prom.h>
#include <o2emu/logging/logger.h>
#include <o2emu/system/bus.h>

namespace o2emu::firmware {

PROM::PROM(o2emu::system::Bus *bus, o2emu::cpu::CPU *cpu)
    : bus_(bus), cpu_(cpu), image_(nullptr), loaded_(false) {}

// Destructor is defaulted in header

bool PROM::load_image(const std::string &path) {
  image_ = std::make_unique<PROMImage>();
  if (!image_->load(path)) {
    image_.reset();
    return false;
  }

  // Map PROM image to memory
  map_to_memory();
  loaded_ = true;

  O2EMU_LOG_INFO_F("PROM image loaded successfully, entry point: 0x%08X",
                   image_->entry_point());
  return true;
}

void PROM::map_to_memory() {
  if (!image_ || !image_->valid())
    return;

  const u8 *data = image_->data();
  u32 size = image_->size();

  // Map PROM at physical address 0x1FC00000 (KSEG1)
  // Also accessible at 0xBFC00000 (reset vector)
  for (u32 i = 0; i < size; ++i) {
    bus_->write8(0x1FC00000 + i, data[i]);
    bus_->write8(0xBFC00000 + i, data[i]);
  }

  // Also map at VMA 0x81000000 (firmware VMA)
  for (u32 i = 0; i < size; ++i) {
    bus_->write8(0x81000000 + i, data[i]);
  }
}

void PROM::execute() {
  if (!loaded_ || !image_) {
    O2EMU_LOG_ERROR_F("No PROM image loaded");
    return;
  }

  u32 entry = image_->entry_point();
  if (entry == 0) {
    entry = 0xBFC00000; // Default reset vector
  }

  O2EMU_LOG_INFO_F("Starting PROM execution at 0x%08X", entry);

  // Set CPU PC to entry point
  cpu_->state().gpr[31] = 0; // RA = 0
  cpu_->cp0().set_epc(entry);
  cpu_->cp0().set_status(0x00400004); // BEV=1, KSU=kernel

  // Start execution
  cpu_->state().gpr[0] = 0; // Zero register
}

void PROM::reset() {
  if (loaded_ && image_) {
    // Re-map PROM to memory
    map_to_memory();

    // Reset CPU to PROM entry point
    execute();
  }
}

// is_loaded() and image() are defined inline in header

u32 PROM::entry_point() const {
  return image_ ? image_->entry_point() : 0xBFC00000;
}

// PROMImage implementation
bool PROMImage::load(const std::string &path) {
  // Read file into buffer
  FILE *f = fopen(path.c_str(), "rb");
  if (!f) {
    O2EMU_LOG_ERROR_F("Failed to open PROM image: %s", path.c_str());
    return false;
  }

  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);

  if (size <= 0) {
    fclose(f);
    return false;
  }

  image_.resize(size);
  size_t read = fread(image_.data(), 1, size, f);
  fclose(f);

  if (read != static_cast<size_t>(size)) {
    O2EMU_LOG_ERROR_F("Failed to read PROM image: %s", path.c_str());
    return false;
  }

  // Parse SHDR sections
  if (!parse_shdr_sections()) {
    O2EMU_LOG_ERROR("Invalid SHDR sections in PROM image");
    return false;
  }

  // Parse embedded ELF from firmware section
  if (!parse_elf()) {
    O2EMU_LOG_WARN("No embedded ELF found in PROM image");
  }

  return true;
}

bool PROMImage::load_from_buffer(const u8 *data, size_t size) {
  if (!data || size == 0) {
    return false;
  }

  image_.assign(data, data + size);

  if (!parse_shdr_sections()) {
    O2EMU_LOG_ERROR("Invalid SHDR sections in PROM buffer");
    return false;
  }

  if (!parse_elf()) {
    O2EMU_LOG_WARN("No embedded ELF found in PROM buffer");
  }

  return true;
}

bool PROMImage::parse_shdr_sections() {
  static constexpr size_t kShdrHeaderSize =
      64; // SHDR_SIZE from decompiled PROM definitions.h
  static constexpr u32 kShdrMagic =
      0x52444853; // "SHDR" in little-endian (bytes: 52 44 48 53 = 'R' 'D' 'H'
                  // 'S')

  // Known SHDR header offsets in the PROM file (from decompiled PROM)
  static constexpr std::array<u32, 5> kShdrOffsets = {0x8, 0x4000, 0x4400,
                                                      0x9200, 0x69200};

  // Helper to read big-endian u32 from memory
  auto read_be32 = [](const u8 *ptr) -> u32 {
    return (static_cast<u32>(ptr[0]) << 24) | (static_cast<u32>(ptr[1]) << 16) |
           (static_cast<u32>(ptr[2]) << 8) | static_cast<u32>(ptr[3]);
  };

  // Helper to read big-endian u16 from memory
  auto read_be16 = [](const u8 *ptr) -> u16 {
    return (static_cast<u16>(ptr[0]) << 8) | static_cast<u16>(ptr[1]);
  };

  if (image_.size() < kShdrOffsets.back() + kShdrHeaderSize) {
    O2EMU_LOG_ERROR("PROM image too small for SHDR headers");
    return false;
  }

  sections_.clear();

  // Check each known SHDR offset
  for (u32 shdr_offset : kShdrOffsets) {
    if (shdr_offset + kShdrHeaderSize > image_.size()) {
      O2EMU_LOG_WARN_F("SHDR offset 0x%08X exceeds file size", shdr_offset);
      continue;
    }

    const u8 *shdr_ptr = image_.data() + shdr_offset;

    // Check SHDR magic (stored as little-endian in the file)
    u32 magic = read_be32(shdr_ptr);
    if (magic != kShdrMagic) {
      O2EMU_LOG_WARN_F("Invalid SHDR magic at offset 0x%08X: 0x%08X",
                       shdr_offset, magic);
      continue;
    }

    // Parse SHDR header fields (all big-endian in the file)
    u32 section_len = read_be32(shdr_ptr + 4);
    u16 name_len = read_be16(shdr_ptr + 8);
    u16 version_len = read_be16(shdr_ptr + 10);
    u8 section_type = shdr_ptr[12];
    // padding[3] at 13-15
    const char *name = reinterpret_cast<const char *>(shdr_ptr + 16);
    const char *version = reinterpret_cast<const char *>(shdr_ptr + 48);
    // checksum at 56-59, reserved[4] at 60-63

    // Section data starts right after the SHDR header (at shdr_offset + 64)
    size_t section_data_offset = shdr_offset + kShdrHeaderSize;

    // Validate section bounds
    if (section_data_offset + section_len > image_.size()) {
      O2EMU_LOG_WARN_F("Section at SHDR offset 0x%08X exceeds file size "
                       "(data_offset=%zu, size=%u)",
                       shdr_offset, section_data_offset, section_len);
      continue;
    }

    // Extract name and version (use lengths from header, max 32 and 8)
    size_t actual_name_len = std::min<size_t>(name_len, 32);
    size_t actual_version_len = std::min<size_t>(version_len, 8);
    std::string section_name(name, actual_name_len);
    std::string section_version(version, actual_version_len);

    SectionInfo info;
    info.type = section_type;
    info.offset = section_data_offset; // Section data starts after SHDR header
    info.size = section_len;
    info.name = section_name;
    info.version = section_version;

    O2EMU_LOG_DEBUG_F(
        "Found SHDR section: name='%s', version='%s', type=0x%02X, "
        "data_offset=%zu, size=%u",
        info.name.c_str(), info.version.c_str(), info.type, info.offset,
        info.size);

    sections_.push_back(info);
  }

  if (sections_.empty()) {
    O2EMU_LOG_ERROR("No valid SHDR sections found");
    return false;
  }

  // Sort sections by offset (should already be in order, but just in case)
  std::sort(sections_.begin(), sections_.end(),
            [](const SectionInfo &a, const SectionInfo &b) {
              return a.offset < b.offset;
            });

  O2EMU_LOG_INFO_F("Parsed %zu SHDR sections", sections_.size());
  return true;
}

bool PROMImage::parse_elf() {
  // Find the firmware section (type = SECTION_TYPE_CODE | SECTION_TYPE_LOADABLE
  // = 5) Actually from decompiled PROM, firmware has type 3 (CODE | DATA) Let's
  // search for a section containing an ELF header
  for (const auto &sect : sections_) {
    if (sect.size < sizeof(ELFHeader)) {
      continue;
    }

    const ELFHeader *elf =
        reinterpret_cast<const ELFHeader *>(image_.data() + sect.offset);

    // Check ELF magic: 0x7F 'E' 'L' 'F'
    if (elf->ident[0] == 0x7F && elf->ident[1] == 'E' && elf->ident[2] == 'L' &&
        elf->ident[3] == 'F') {
      entry_point_ = elf->entry;
      O2EMU_LOG_INFO_F(
          "Found embedded ELF in section '%s', entry point: 0x%08X",
          sect.name.c_str(), entry_point_);
      return true;
    }
  }

  return false;
}

u32 PROMImage::compute_checksum(const u8 *data, size_t size) {
  u32 sum = 0;
  for (size_t i = 0; i < size; i += 4) {
    u32 word = 0;
    if (i + 3 < size) {
      word = (static_cast<u32>(data[i]) << 24) |
             (static_cast<u32>(data[i + 1]) << 16) |
             (static_cast<u32>(data[i + 2]) << 8) |
             static_cast<u32>(data[i + 3]);
    } else {
      // Handle partial word at end
      for (size_t j = 0; j < 4 && (i + j) < size; ++j) {
        word |= static_cast<u32>(data[i + j]) << (24 - j * 8);
      }
    }
    sum += word;
  }
  return sum;
}

} // namespace o2emu::firmware