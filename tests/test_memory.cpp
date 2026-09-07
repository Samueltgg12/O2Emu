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
  Memory mem(0x10000);
  MRE mre(mem);

  // Interface-buffer control resets to empty (not full).
  EXPECT_EQ(mre.read(MRE::INTFBUF_CTL), 0x00000000);

  // A generic register write/read round-trips.
  mre.write(MRE::PIXPIPE_DRAWMODE, 0xDEADBEEF);
  EXPECT_EQ(mre.read(MRE::PIXPIPE_DRAWMODE), 0xDEADBEEF);
}

TEST(MRE, InterfaceBufferReset) {
  Memory mem(0x10000);
  MRE mre(mem);

  // Program the interface-buffer control, then reset it.
  mre.write(MRE::INTFBUF_CTL, 0xFFFFFFFF);
  mre.write(MRE::INTFBUF_RESET, 0x1);
  EXPECT_EQ(mre.read(MRE::INTFBUF_CTL), 0x00000000);
}

TEST(MRE, RenderLifecycle) {
  Memory mem(0x10000);
  MRE mre(mem);

  // Setting the start pointer begins rendering.
  mre.write(MRE::SET_START_PTR, 0x81000000);
  EXPECT_TRUE(mre.render_busy());

  // Flushing the pixel pipe completes the render.
  mre.write(MRE::PIXPIPE_FLUSH, 0x1);
  EXPECT_FALSE(mre.render_busy());
}

TEST(MRE, MTETransfer) {
  Memory mem(0x10000);
  MRE mre(mem);

  // Start a memory-transfer-engine operation.
  mre.start_dma(0x100000, 0x200000, 0x1000);
  EXPECT_TRUE(mre.dma_busy());

  // Flushing the MTE completes the transfer.
  mre.write(MRE::MTE_FLUSH, 0x1);
  EXPECT_FALSE(mre.dma_busy());
}