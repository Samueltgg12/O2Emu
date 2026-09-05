/**
 * @file rtc.cpp
 * @brief Real-Time Clock (DS1286/DS1386 compatible) implementation
 */

#include <cstring>
#include <ctime>
#include <o2emu/devices/rtc.h>
#include <o2emu/logging/logger.h>

namespace o2emu::devices {

RTC::RTC()
    : Device("RTC", 0xC0000000, 0x1000) // ISA I/O space
      ,
      cmos_ram_(nullptr) {
  reset();
}

RTC::~RTC() { delete[] cmos_ram_; }

void RTC::reset() {
  Device::reset();
  std::memset(regs_.data(), 0, regs_.size());

  // Allocate CMOS RAM (128 bytes)
  if (!cmos_ram_) {
    cmos_ram_ = new u8[128];
  }
  std::memset(cmos_ram_, 0, 128);

  // Initialize with current time
  update_time();

  // RTC register defaults (DS1286/DS1386 compatible)
  // Register A: 0x0A
  regs_[REG_A] = 0x26; // 32.768kHz oscillator, 1024Hz periodic

  // Register B: 0x0B
  regs_[REG_B] = 0x02; // 24-hour mode, binary, no updates

  // Register C: 0x0C (read-only)
  regs_[REG_C] = 0x00;

  // Register D: 0x0D (read-only)
  regs_[REG_D] = 0x80; // Valid RAM and time
}

void RTC::update_time() {
  std::time_t now = std::time(nullptr);
  std::tm *tm = std::localtime(&now);

  if (tm) {
    cmos_ram_[0x00] = bcd(tm->tm_sec);        // Seconds
    cmos_ram_[0x02] = bcd(tm->tm_min);        // Minutes
    cmos_ram_[0x04] = bcd(tm->tm_hour);       // Hours
    cmos_ram_[0x06] = bcd(tm->tm_wday + 1);   // Day of week (1-7)
    cmos_ram_[0x07] = bcd(tm->tm_mday);       // Day of month
    cmos_ram_[0x08] = bcd(tm->tm_mon + 1);    // Month
    cmos_ram_[0x09] = bcd(tm->tm_year % 100); // Year (2-digit)
    cmos_ram_[0x32] = bcd(tm->tm_year / 100); // Century
  }
}

u8 RTC::bcd(int value) {
  return static_cast<u8>((value / 10) << 4 | (value % 10));
}

int RTC::from_bcd(u8 value) { return ((value >> 4) * 10) + (value & 0x0F); }

u32 RTC::read_reg(u32 offset) {
  if (offset < 0x10) {
    // Standard RTC registers
    switch (offset) {
    case 0x00:
    case 0x01:
    case 0x02:
    case 0x03:
    case 0x04:
    case 0x05:
    case 0x06:
    case 0x07:
    case 0x08:
    case 0x09:
    case 0x0A:
    case 0x0B:
    case 0x0C:
    case 0x0D:
      return regs_[offset];
    default:
      return 0;
    }
  } else if (offset < 0x80) {
    // Extended registers
    return regs_[offset];
  } else if (offset < 0x100) {
    // CMOS RAM
    return cmos_ram_[offset - 0x80];
  }
  return 0;
}

void RTC::write_reg(u32 offset, u32 value) {
  if (offset < 0x10) {
    // Standard RTC registers
    switch (offset) {
    case REG_A:
      regs_[offset] = value & 0x7F; // Bit 7 is read-only (UIP)
      break;
    case REG_B:
      regs_[offset] = value;
      break;
    case REG_C:
    case REG_D:
      // Read-only
      break;
    default:
      regs_[offset] = value;
      break;
    }
  } else if (offset < 0x80) {
    // Extended registers
    regs_[offset] = value;
  } else if (offset < 0x100) {
    // CMOS RAM
    cmos_ram_[offset - 0x80] = value;
  }
}

bool RTC::read([[maybe_unused]] u32 offset, [[maybe_unused]] u32 size,
               u32 &value) {
  if (offset < sizeof(regs_) + 128) {
    value = read_reg(offset);
    return true;
  }
  return false;
}

bool RTC::write([[maybe_unused]] u32 offset, [[maybe_unused]] u32 size,
                u32 value) {
  if (offset < sizeof(regs_) + 128) {
    write_reg(offset, value);
    return true;
  }
  return false;
}

void RTC::tick(u64 cycles) {
  // Update time once per second (approximately)
  static u64 cycle_counter = 0;
  cycle_counter += cycles;

  if (cycle_counter >= 133000000) { // ~1 second at 133MHz
    cycle_counter = 0;
    update_time();

    // Generate periodic interrupt if enabled
    if (regs_[REG_B] & 0x40) { // PIE - Periodic Interrupt Enable
      regs_[REG_C] |= 0x40;    // PF - Periodic Flag
    }

    // Generate update-ended interrupt if enabled
    if (regs_[REG_B] & 0x10) { // UIE - Update-ended Interrupt Enable
      regs_[REG_C] |= 0x10;    // UF - Update-ended Flag
    }

    // Generate alarm interrupt if enabled
    if (regs_[REG_B] & 0x20) { // AIE - Alarm Interrupt Enable
      // Check alarm match (simplified)
      regs_[REG_C] |= 0x20; // AF - Alarm Flag
    }
  }
}

u32 RTC::interrupt_status() const {
  return regs_[REG_C] & (regs_[REG_B] & 0x70); // Only enabled interrupts
}

} // namespace o2emu::devices