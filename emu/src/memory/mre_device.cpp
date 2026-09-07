#include <o2emu/memory/mre_device.h>

namespace o2emu::memory {

MREDevice::MREDevice(MRE &mre)
    : devices::Device("MRE Render Engine", 0x15000000, 0x10000), mre_(mre) {}

u32 MREDevice::read32(u32 offset) { return mre_.read(offset & ~3u); }

u16 MREDevice::read16(u32 offset) {
  const u32 value = read32(offset);
  return static_cast<u16>(value >> ((offset & 2u) * 8u));
}

u8 MREDevice::read8(u32 offset) {
  const u32 value = read32(offset);
  return static_cast<u8>(value >> ((offset & 3u) * 8u));
}

void MREDevice::write32(u32 offset, u32 value) {
  mre_.write(offset & ~3u, value);
}

void MREDevice::write16(u32 offset, u16 value) {
  const u32 aligned = offset & ~3u;
  u32 current = mre_.read(aligned);
  const u32 shift = (offset & 2u) * 8u;
  current =
      (current & ~(0xffffu << shift)) | (static_cast<u32>(value) << shift);
  mre_.write(aligned, current);
}

void MREDevice::write8(u32 offset, u8 value) {
  const u32 aligned = offset & ~3u;
  u32 current = mre_.read(aligned);
  const u32 shift = (offset & 3u) * 8u;
  current = (current & ~(0xffu << shift)) | (static_cast<u32>(value) << shift);
  mre_.write(aligned, current);
}

void MREDevice::reset() { mre_.reset(); }

} // namespace o2emu::memory