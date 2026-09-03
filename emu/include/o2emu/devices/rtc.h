/**
 * @file rtc.h
 * @brief Real-Time Clock (DS1286/DS1386 compatible) interface
 *
 * Based on Dallas Semiconductor DS1286/DS1386 datasheet and IRIX RTC driver
 * O2 uses DS1286/DS1386 compatible RTC with 128 bytes CMOS RAM
 */

#pragma once

#include <array>
#include <o2emu/devices/device.h>
#include <o2emu/o2emu.h>

namespace o2emu::devices {

class RTC : public Device {
public:
  RTC();
  ~RTC() override;

  // RTC register offsets (DS1286/DS1386 compatible)
  enum Register : uint32_t {
    // Standard RTC registers (0x00-0x0D)
    REG_SECONDS = 0x00,
    REG_SECONDS_ALARM = 0x01,
    REG_MINUTES = 0x02,
    REG_MINUTES_ALARM = 0x03,
    REG_HOURS = 0x04,
    REG_HOURS_ALARM = 0x05,
    REG_DAY_OF_WEEK = 0x06,
    REG_DAY_OF_MONTH = 0x07,
    REG_MONTH = 0x08,
    REG_YEAR = 0x09,
    REG_A = 0x0A,
    REG_B = 0x0B,
    REG_C = 0x0C,
    REG_D = 0x0D,

    // Extended registers (0x0E-0x7F)
    // ...

    // CMOS RAM (0x80-0xFF)
    CMOS_BASE = 0x80,
    CMOS_SIZE = 128,
  };

  // Register A bits
  enum RegABit : uint8_t {
    REGA_UIP = 0x80, // Update In Progress (read-only)
    REGA_DV = 0x70,  // Divider bits
    REGA_RS = 0x0F,  // Rate Select bits
  };

  // Register B bits
  enum RegBBit : uint8_t {
    REGB_SET = 0x80,  // Update cycle inhibit
    REGB_PIE = 0x40,  // Periodic Interrupt Enable
    REGB_AIE = 0x20,  // Alarm Interrupt Enable
    REGB_UIE = 0x10,  // Update-ended Interrupt Enable
    REGB_SQWE = 0x08, // Square Wave Enable
    REGB_DM = 0x04,   // Data Mode (0=BCD, 1=Binary)
    REGB_24 = 0x02,   // 24-hour mode (1=24hr, 0=12hr)
    REGB_DSE = 0x01,  // Daylight Savings Enable
  };

  // Register C bits (read-only)
  enum RegCBit : uint8_t {
    REGC_IRQF = 0x80, // Interrupt Request Flag
    REGC_PF = 0x40,   // Periodic Flag
    REGC_AF = 0x20,   // Alarm Flag
    REGC_UF = 0x10,   // Update-ended Flag
  };

  // Register D bits (read-only)
  enum RegDBit : uint8_t {
    REGD_VRT = 0x80, // Valid RAM and Time
  };

  // Device interface
  u32 read32(u32 offset) override;
  u16 read16(u32 offset) override;
  u8 read8(u32 offset) override;

  void write32(u32 offset, u32 value) override;
  void write16(u32 offset, u16 value) override;
  void write8(u32 offset, u8 value) override;

  void reset() override;
  void tick(u64 cycles) override;

  u32 interrupt_status() const;

  // Generic read/write for bus interface
  bool read(u32 offset, u32 size, u32 &value);
  bool write(u32 offset, u32 size, u32 value);

private:
  std::array<u8, 256> regs_ = {};
  u8 *cmos_ram_ = nullptr;

  u32 read_reg(u32 offset);
  void write_reg(u32 offset, u32 value);
  void update_time();
  u8 bcd(int value);
  int from_bcd(u8 value);
};

} // namespace o2emu::devices