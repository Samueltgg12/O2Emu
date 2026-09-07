/**
 * @file crime_device.cpp
 * @brief Bus-facing CRIME device adapter implementation
 */

#include <o2emu/memory/crime_device.h>

namespace o2emu::memory {

CRIMEDevice::CRIMEDevice(CRIME &crime)
    : Device("CRIME", ip32::PHYS_BASE_CRIME, 0x10000), crime_(crime) {}

u32 CRIMEDevice::read32(u32 offset) {
  return crime_.read(offset);
}

u16 CRIMEDevice::read16(u32 offset) {
  u32 aligned = offset & ~3;
  u32 val = crime_.read(aligned);
  int shift = (2 - (offset & 2)) * 8;
  return static_cast<u16>((val >> shift) & 0xFFFF);
}

u8 CRIMEDevice::read8(u32 offset) {
  u32 aligned = offset & ~3;
  u32 val = crime_.read(aligned);
  int shift = (3 - (offset & 3)) * 8;
  return static_cast<u8>((val >> shift) & 0xFF);
}

void CRIMEDevice::write32(u32 offset, u32 value) {
  crime_.write(offset, value);
}

void CRIMEDevice::write16(u32 offset, u16 value) {
  u32 aligned = offset & ~3;
  u32 cur = crime_.read(aligned);
  int shift = (2 - (offset & 2)) * 8;
  u32 mask = ~(0xFFFFu << shift);
  u32 new_val = (cur & mask) | ((static_cast<u32>(value) & 0xFFFFu) << shift);
  crime_.write(aligned, new_val);
}

void CRIMEDevice::write8(u32 offset, u8 value) {
  u32 aligned = offset & ~3;
  u32 cur = crime_.read(aligned);
  int shift = (3 - (offset & 3)) * 8;
  u32 mask = ~(0xFFu << shift);
  u32 new_val = (cur & mask) | ((static_cast<u32>(value) & 0xFFu) << shift);
  crime_.write(aligned, new_val);
}

void CRIMEDevice::reset() {
  crime_.reset();
}

} // namespace o2emu::memory
