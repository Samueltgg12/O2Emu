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

} // namespace o2emu::firmware