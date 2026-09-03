#pragma once

/**
 * @file mace.h
 * @brief MACE (Multimedia, Audio and Communications Engine) ASIC
 *
 * Base address: 0x1F000000 (PHYS_BASE_MACE)
 * Based on Linux arch/mips/include/asm/ip32/mace.h and IRIX sources
 */

#include <memory>
#include <o2emu/devices/device.h>
#include <o2emu/o2emu.h>

namespace o2emu::devices {

class MACEPCI;
class MACEVideo;
class MACEEthernet;
class MACEAudio;
class MACEISA;

class MACE : public Device {
public:
  MACE();
  ~MACE() override;

  // Sub-devices
  MACEPCI &pci() { return *pci_; }
  MACEVideo &video() { return *video_; }
  MACEEthernet &ethernet() { return *ethernet_; }
  MACEAudio &audio() { return *audio_; }
  MACEISA &isa() { return *isa_; }

  // Device interface
  u32 read32(u32 offset) override;
  u16 read16(u32 offset) override;
  u8 read8(u32 offset) override;

  void write32(u32 offset, u32 value) override;
  void write16(u32 offset, u16 value) override;
  void write8(u32 offset, u8 value) override;

  void reset() override;
  void tick(u64 cycles) override;

  // Interrupt routing
  void set_interrupt(int line, bool asserted);

private:
  std::unique_ptr<MACEPCI> pci_;
  std::unique_ptr<MACEVideo> video_;
  std::unique_ptr<MACEEthernet> ethernet_;
  std::unique_ptr<MACEAudio> audio_;
  std::unique_ptr<MACEISA> isa_;

  // Interrupt state
  u32 interrupt_status_ = 0;
  u32 interrupt_mask_ = 0;
};

} // namespace o2emu::devices