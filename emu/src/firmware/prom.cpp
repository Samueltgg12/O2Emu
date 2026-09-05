/**
 * @file prom.cpp
 * @brief PROM firmware execution
 */

#include <cstring>
#include <o2emu/firmware/prom.h>
#include <o2emu/logging/logger.h>

namespace o2emu::firmware {

PROM::PROM(Bus *bus, CPU *cpu)
    : bus_(bus), cpu_(cpu), image_(nullptr), loaded_(false) {}

PROM::~PROM() = default;

bool PROM::load_image(const std::string &path) {
  image_ = std::make_unique<PROMImage>();
  if (!image_->load(path)) {
    image_.reset();
    return false;
  }

  // Map PROM image to memory
  map_to_memory();
  loaded_ = true;

  O2EMU_LOG_INFO("PROM image loaded successfully, entry point: 0x"
                 << std::hex << image_->entry_point() << std::dec);
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
    bus_->write(0x1FC00000 + i, 1, data[i]);
    bus_->write(0xBFC00000 + i, 1, data[i]);
  }

  // Also map at VMA 0x81000000 (firmware VMA)
  for (u32 i = 0; i < size; ++i) {
    bus_->write(0x81000000 + i, 1, data[i]);
  }
}

void PROM::execute() {
  if (!loaded_ || !image_) {
    O2EMU_LOG_ERROR("No PROM image loaded");
    return;
  }

  u32 entry = image_->entry_point();
  if (entry == 0) {
    entry = 0xBFC00000; // Default reset vector
  }

  O2EMU_LOG_INFO("Starting PROM execution at 0x" << std::hex << entry
                                                 << std::dec);

  // Set CPU PC to entry point
  cpu_->set_gpr(31, 0); // RA = 0
  cpu_->set_cp0_reg(CP0_EPC, entry);
  cpu_->set_cp0_reg(CP0_STATUS, 0x00400004); // BEV=1, KSU=kernel

  // Start execution
  cpu_->set_gpr(0, 0); // Zero register
}

void PROM::reset() {
  if (loaded_ && image_) {
    // Re-map PROM to memory
    map_to_memory();

    // Reset CPU to PROM entry point
    execute();
  }
}

bool PROM::is_loaded() const { return loaded_; }

u32 PROM::entry_point() const {
  if (image_)
    return image_->entry_point();
  return 0;
}

const PROMImage *PROM::image() const { return image_.get(); }

// PROMImage implementation
bool PROMImage::load(const std::string &path) {
  // Read file into buffer
  FILE *f = fopen(path.c_str(), "rb");
  if (!f) {
    O2EMU_LOG_ERROR("Failed to open PROM image: " << path);
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
    O2EMU_LOG_ERROR("Failed to read PROM image: " << path);
    return false;
  }

  // Parse SHDR header
  if (!parse_shdr()) {
    O2EMU_LOG_ERROR("Invalid SHDR header in PROM image");
    return false;
  }

  // Parse embedded ELF
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

  if (!parse_shdr()) {
    O2EMU_LOG_ERROR("Invalid SHDR header in PROM buffer");
    return false;
  }

  if (!parse_elf()) {
    O2EMU_LOG_WARN("No embedded ELF found in PROM buffer");
  }

  return true;
}

bool PROMImage::parse_shdr() {
  if (image_.size() < sizeof(SHDRHeader)) {
    return false;
  }

  const SHDRHeader *shdr = reinterpret_cast<const SHDRHeader *>(image_.data());

  // Check magic "SHDR" (0x48445253)
  if (shdr->magic != 0x48445253) {
    O2EMU_LOG_ERROR("Invalid SHDR magic: 0x" << std::hex << shdr->magic
                                             << std::dec);
    return false;
  }

  // Verify checksum
  u32 computed = compute_checksum(image_.data(), image_.size());
  if (computed != 0) {
    O2EMU_LOG_WARN("SHDR checksum mismatch: computed 0x" << std::hex << computed
                                                         << std::dec);
  }

  return true;
}

bool PROMImage::parse_elf() {
  if (image_.size() < sizeof(SHDRHeader)) {
    return false;
  }

  const SHDRHeader *shdr = reinterpret_cast<const SHDRHeader *>(image_.data());

  // Check if we have enough sections
  if (shdr->num_sections < 5) {
    return false;
  }

  // Section table follows header
  const SHDRSection *sections = reinterpret_cast<const SHDRSection *>(
      image_.data() + shdr->section_offset);

  // Section 4 should be the embedded ELF
  if (shdr->num_sections > 4) {
    const SHDRSection &elf_sect = sections[4];
    if (elf_sect.type == SECT_ELF && elf_sect.size >= sizeof(ELFHeader)) {
      const ELFHeader *elf = reinterpret_cast<const ELFHeader *>(
          image_.data() + elf_sect.file_offset);

      // Check ELF magic
      if (elf->ident[0] == 0x7F && elf->ident[1] == 'E' &&
          elf->ident[2] == 'L' && elf->ident[3] == 'F') {
        entry_point_ = elf->entry;
        return true;
      }
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