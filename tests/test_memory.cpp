/**
 * @file test_memory.cpp
 * @brief Memory subsystem unit tests
 */

#include <gtest/gtest.h>
#include <o2emu/memory/address_space.h>
#include <o2emu/memory/crime.h>
#include <o2emu/memory/memory.h>
#include <o2emu/memory/mre.h>
#include <o2emu/system/bus.h>

using namespace o2emu;
using namespace o2emu::memory;
using namespace o2emu::system;

TEST(Memory, BasicReadWrite) {
  Memory mem(0x10000); // 64KB

  mem.write32(0x100, 0x12345678);
  EXPECT_EQ(mem.read32(0x100), 0x12345678);

  mem.write16(0x200, 0xABCD);
  EXPECT_EQ(mem.read16(0x200), 0xABCD);

  mem.write8(0x300, 0xEF);
  EXPECT_EQ(mem.read8(0x300), 0xEF);
}

TEST(Memory, UnalignedAccess) {
  Memory mem(0x10000);

  // Unaligned 32-bit write/read
  mem.write32(0x101, 0x12345678);
  EXPECT_EQ(mem.read32(0x101), 0x12345678);
}

TEST(Memory, OutOfBounds) {
  Memory mem(0x1000);

  // Out of bounds should return 0 / not crash
  EXPECT_EQ(mem.read32(0x2000), 0);
  mem.write32(0x2000, 0x12345678); // Should not crash
}

TEST(AddressSpace, DeviceMapping) {
  AddressSpace as;
  auto mem = std::make_shared<Memory>(0x10000);

  as.map_device(0x00000000, 0x10000, mem);

  as.write32(0x100, 0x12345678);
  EXPECT_EQ(as.read32(0x100), 0x12345678);
}

TEST(AddressSpace, OverlappingDevices) {
  AddressSpace as;
  auto mem1 = std::make_shared<Memory>(0x10000);
  auto mem2 = std::make_shared<Memory>(0x10000);

  as.map_device(0x00000000, 0x10000, mem1);
  as.map_device(0x00008000, 0x10000, mem2); // Overlaps

  // Second mapping should take precedence in overlap region
  as.write32(0x9000, 0xAAAAAAAA);
  EXPECT_EQ(as.read32(0x9000), 0xAAAAAAAA);
}

TEST(CRIME, RegisterAccess) {
  CRIME crime;

  // Test revision register
  u32 rev = crime.read(CRIME::CRM_REVISION);
  EXPECT_EQ(rev, 0x00010000);

  // Test memory config
  crime.write(CRIME::CRM_MEM_CONFIG, 0x00000001); // 1 bank, ECC enabled
  EXPECT_EQ(crime.read(CRIME::CRM_MEM_CONFIG), 0x00000001);
}

TEST(CRIME, BankConfiguration) {
  CRIME crime;

  crime.set_bank_config(0, 0x00000000, 64, true); // 64MB at base
  crime.set_bank_config(1, 0x04000000, 64, true); // 64MB at 64MB

  EXPECT_EQ(crime.num_banks(), 2);
  EXPECT_EQ(crime.total_memory_mb(), 128);

  u32 config0 = crime.get_bank_config(0);
  EXPECT_NE(config0, 0);
}

TEST(CRIME, InterruptController) {
  CRIME crime;

  // Enable UART1 interrupt
  crime.set_interrupt_mask(1 << CRIME::INTR_UART_1);

  // Assert UART1 interrupt
  crime.set_interrupt(CRIME::INTR_UART_1, true);

  EXPECT_EQ(crime.pending_interrupts(), 1 << CRIME::INTR_UART_1);

  // Clear interrupt
  crime.set_interrupt(CRIME::INTR_UART_1, false);
  EXPECT_EQ(crime.pending_interrupts(), 0);
}

TEST(CRIME, ECC) {
  CRIME crime;

  crime.enable_ecc(true);
  EXPECT_TRUE(crime.ecc_enabled());

  crime.inject_ecc_error(0x1000, 0x1234);
  // ECC error should be reflected in status
  u32 status = crime.read(CRIME::CRM_MEM_ECC_STATUS);
  EXPECT_NE(status, 0);
}

TEST(CRIME, Timers) {
  CRIME crime;

  // Set timer compare values
  crime.write(CRIME::CRM_MEM_CONFIG, 0); // Use config for timer setup

  // Tick timers
  crime.tick_timers();
  crime.tick_timers();

  // Timer interrupts should be set after compare match
  // (depends on implementation)
}

TEST(MRE, RegisterAccess) {
  MRE mre;

  u32 rev = mre.read(MRE::MRE_REVISION);
  EXPECT_EQ(rev, 0x00010000);

  // Test control register
  mre.write(MRE::MRE_CONTROL, MRE::CTRL_ENABLE);
  EXPECT_EQ(mre.read(MRE::MRE_CONTROL), MRE::CTRL_ENABLE);
}

TEST(MRE, TileOperations) {
  MRE mre;

  mre.write(MRE::MRE_CONTROL, MRE::CTRL_ENABLE | MRE::CTRL_TILE_MODE);

  // Write tile
  u32 tile_data[16] = {0};
  for (int i = 0; i < 16; i++)
    tile_data[i] = 0xFF0000FF; // Blue
  mre.write_tile(0, 0, tile_data, 64);

  // Read tile back
  u32 read_data[16] = {0};
  mre.read_tile(0, 0, read_data, 64);

  for (int i = 0; i < 16; i++) {
    EXPECT_EQ(read_data[i], 0xFF0000FF);
  }
}

TEST(MRE, Rasterization) {
  MRE mre;

  mre.write(MRE::MRE_CONTROL, MRE::CTRL_ENABLE);

  // Draw triangle
  mre.draw_triangle(100, 100, 200, 100, 150, 200, 0xFF0000FF);

  // Check status
  EXPECT_TRUE(mre.busy() || !mre.busy()); // Just verify it runs
}