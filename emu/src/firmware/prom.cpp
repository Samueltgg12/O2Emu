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
  // The first SHDR header is at offset 8 (after initial branch + nop)
  constexpr size_t kFirstShdrOffset = 8;
  constexpr size_t kShdrHeaderSize = 64; // SHDR_SIZE from definitions.h

  if (image_.size() < kFirstShdrOffset + kShdrHeaderSize) {
    O2EMU_LOG_ERROR("PROM image too small for SHDR header");
    return false;
  }

  size_t offset = kFirstShdrOffset;
  sections_.clear();

  while (offset + kShdrHeaderSize <= image_.size()) {
    const SHDRSectionHeader *shdr =
        reinterpret_cast<const SHDRSectionHeader *>(image_.data() + offset);

    // Check magic: "SHDR" = 0x53484452 (big-endian), reads as 0x52444853 on LE
    if (shdr->magic != 0x52444853) {
      // Not a valid SHDR header, stop parsing
      break;
    }

    // Extract section info
    SectionInfo info;
    info.type = shdr->section_type;
    info.offset = offset + kShdrHeaderSize;
    info.size = shdr->section_len;

    // Validate section bounds
    if (info.offset + info.size > image_.size()) {
      O2EMU_LOG_WARN_F("Section at offset %zu exceeds file size", offset);
      break;
    }

    // Extract name and version (null-terminated)
    info.name = std::string(shdr->name, strnlen(shdr->name, 32));
    info.version = std::string(shdr->version, strnlen(shdr->version, 8));

    O2EMU_LOG_DEBUG_F(
        "Found SHDR section: name='%s', version='%s', type=0x%02X, "
        "offset=%zu, size=%u",
        info.name.c_str(), info.version.c_str(), info.type, info.offset,
        info.size);

    sections_.push_back(info);

    // Move to next SHDR header (aligned to 64 bytes)
    offset += kShdrHeaderSize + info.size;
    // Align to 64-byte boundary
    offset = (offset + kShdrHeaderSize - 1) & ~(kShdrHeaderSize - 1);
  }

  if (sections_.empty()) {
    O2EMU_LOG_ERROR("No valid SHDR sections found");
    return false;
  }

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