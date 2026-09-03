#pragma once

/**
 * @file o2emu.h
 * @brief Main O2Emu header - core types and constants
 */

#include <cstddef>
#include <cstdint>

namespace o2emu {

// Version
constexpr const char *VERSION = "0.1.0";
constexpr const char *PROJECT_NAME = "O2Emu";

// SGI O2 (IP32) constants
namespace ip32 {

// Physical address map (from decompiled PROM definitions.h)
constexpr uint32_t PHYS_BASE_CRIME = 0x14000000;  // CRIME CPU interface
constexpr uint32_t PHYS_BASE_RENDER = 0x15000000; // Render engine interface
constexpr uint32_t PHYS_BASE_GBE = 0x16000000;    // GBE (Display Engine)
constexpr uint32_t PHYS_BASE_MACE = 0x1F000000;   // MACE ASIC

// MACE sub-offsets
constexpr uint32_t MACE_PCI_OFFSET = 0x080000;
constexpr uint32_t MACE_VIDEO_OFFSET = 0x100000;
constexpr uint32_t MACE_ETHERNET_OFFSET = 0x280000;
constexpr uint32_t MACE_AUDIO_OFFSET = 0x300000;
constexpr uint32_t MACE_PERIPHERAL_OFFSET = 0x380000;
constexpr uint32_t MACE_ISA_EXTERNAL_OFFSET = 0x380000;

// MACE ISA External sub-offsets
constexpr uint32_t MACE_ISA_UART1_OFFSET = 0x10000;
constexpr uint32_t MACE_ISA_UART2_OFFSET = 0x18000;
constexpr uint32_t MACE_ISA_RTC_OFFSET = 0x20000;

// KSEG1 base (uncached)
constexpr uint32_t KSEG1_BASE = 0xA0000000;

// PROM
constexpr uint32_t PROM_RESET_VECTOR = 0xBFC00000;
constexpr uint32_t PROM_VMA_BASE = 0x81000000;

// Memory
constexpr uint32_t MAX_PHYS_MEMORY = 0x40000000; // 1 GB max

// CPU
constexpr uint32_t CPU_CLOCK_HZ = 180000000; // 180 MHz (R5000)

// UMA bus
constexpr uint32_t UMA_BUS_FREQ_HZ = 133000000; // 133 MHz

} // namespace ip32

// Common types
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

// Endianness helpers (O2 is big-endian MIPS)
constexpr u16 bswap16(u16 x) { return (x >> 8) | (x << 8); }
constexpr u32 bswap32(u32 x) {
  return (x >> 24) | ((x >> 8) & 0xFF00) | ((x << 8) & 0xFF0000) | (x << 24);
}
constexpr u64 bswap64(u64 x) {
  return (x >> 56) | ((x >> 40) & 0xFF00) | ((x >> 24) & 0xFF0000) |
         ((x >> 8) & 0xFF000000) | ((x << 8) & 0xFF00000000) |
         ((x << 24) & 0xFF0000000000) | ((x << 40) & 0xFF000000000000) |
         (x << 56);
}

} // namespace o2emu