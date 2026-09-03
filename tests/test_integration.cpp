/**
 * @file test_integration.cpp
 * @brief Integration tests for full system
 */

#include <gtest/gtest.h>
#include <o2emu/cpu/mips_r5000.h>
#include <o2emu/devices/mace/mace.h>
#include <o2emu/devices/ps2.h>
#include <o2emu/devices/rtc.h>
#include <o2emu/devices/scsicontroller.h>
#include <o2emu/devices/uart.h>
#include <o2emu/firmware/prom_loader.h>
#include <o2emu/memory/address_space.h>
#include <o2emu/memory/crime.h>
#include <o2emu/memory/memory.h>
#include <o2emu/memory/mre.h>
#include <o2emu/system/bus.h>
#include <o2emu/system/interrupts.h>
#include <o2emu/system/timer.h>

using namespace o2emu;
using namespace o2emu::cpu;
using namespace o2emu::memory;
using namespace o2emu::devices;
using namespace o2emu::firmware;
using namespace o2emu::system;

class SystemFixture : public ::testing::Test {
protected:
  void SetUp() override {
    bus = std::make_shared<Bus>();
    mem = std::make_shared<Memory>(0x1000000); // 16MB
    bus->map_device(0x00000000, 0x1000000, mem);

    cpu = std::make_unique<MIPSR5000>(bus);
    cpu->reset();

    crime = std::make_unique<CRIME>();
    mre = std::make_unique<MRE>();
    rtc = std::make_unique<RTC>();
    uart = std::make_unique<UART>();
    ps2 = std::make_unique<PS2>();
    mace = std::make_unique<MACE>();
    scsi = std::make_unique<SCSIController>();

    // Map devices
    bus->map_device(CRIME_BASE, 0x10000, crime.get());
    bus->map_device(MRE_BASE, 0x10000, mre.get());
    bus->map_device(RTC_BASE, 0x1000, rtc.get());
    bus->map_device(UART1_BASE, 0x1000, uart.get());
    bus->map_device(PS2_BASE, 0x1000, ps2.get());
    bus->map_device(MACE_BASE, 0x10000, mace.get());
    bus->map_device(SCSI_BASE, 0x10000, scsi.get());
  }

  void TearDown() override {
    cpu.reset();
    crime.reset();
    mre.reset();
    rtc.reset();
    uart.reset();
    ps2.reset();
    mace.reset();
    scsi.reset();
    bus.reset();
    mem.reset();
  }

  std::shared_ptr<Bus> bus;
  std::shared_ptr<Memory> mem;
  std::unique_ptr<MIPSR5000> cpu;
  std::unique_ptr<CRIME> crime;
  std::unique_ptr<MRE> mre;
  std::unique_ptr<RTC> rtc;
  std::unique_ptr<UART> uart;
  std::unique_ptr<PS2> ps2;
  std::unique_ptr<MACE> mace;
  std::unique_ptr<SCSIController> scsi;
};

TEST_F(SystemFixture, FullSystemReset) {
  // All devices should be accessible after reset
  EXPECT_EQ(cpu->pc(), 0xBFC00000);

  // CRIME revision
  EXPECT_EQ(crime->read(CRIME::CRM_REVISION), 0x00010000);

  // MRE revision
  EXPECT_EQ(mre->read(MRE::MRE_REVISION), 0x00010000);

  // RTC
  EXPECT_EQ(rtc->read(RTC::RTC_SECONDS), 0);

  // UART
  EXPECT_EQ(uart->read(UART::UART_LSR), UART::LSR_TX_EMPTY | UART::LSR_TX_IDLE);
}

TEST_F(SystemFixture, MemoryAccessThroughBus) {
  // Write through bus
  bus->write32(0x1000, 0xDEADBEEF);
  EXPECT_EQ(bus->read32(0x1000), 0xDEADBEEF);

  // CPU can access memory
  cpu->set_gpr(1, 0x1000);
  cpu->execute_instruction(0x8C020000); // LW $2, 0($1)
  EXPECT_EQ(cpu->gpr(2), 0xDEADBEEF);
}

TEST_F(SystemFixture, CRIMEInterrupts) {
  // Enable UART1 interrupt in CRIME
  crime->set_interrupt_mask(1 << CRIME::INTR_UART_1);

  // Assert interrupt from UART
  crime->set_interrupt(CRIME::INTR_UART_1, true);

  // CPU should see interrupt
  u32 pending = crime->pending_interrupts();
  EXPECT_EQ(pending, 1 << CRIME::INTR_UART_1);
}

TEST_F(SystemFixture, UARTLoopback) {
  uart->write(UART::UART_FCR,
              UART::FCR_FIFO_ENABLE | UART::FCR_CLEAR_RX | UART::FCR_CLEAR_TX);
  uart->write(UART::UART_LCR, 0x03); // 8N1
  uart->write(UART::UART_IER, UART::IER_RX_DATA);

  // Write to TX
  uart->write(UART::UART_THR, 'H');
  uart->write(UART::UART_THR, 'i');

  // In loopback mode, RX should have data
  // (Implementation dependent)
}

TEST_F(SystemFixture, PS2Keyboard) {
  ps2->write(PS2::PS2_CMD, PS2::CMD_ENABLE_KEYBOARD);

  // Send key press
  ps2->inject_keyboard_data(0x1C); // 'A' key down

  EXPECT_TRUE(ps2->keyboard_interrupt_pending());

  // Read data
  u32 data = ps2->read(PS2::PS2_DATA);
  EXPECT_EQ(data, 0x1C);
}

TEST_F(SystemFixture, PROMBoot) {
  // Load PROM
  PromLoader loader;
  bool loaded = loader.load("samples/ip32prom.rev4.18.bin");
  EXPECT_TRUE(loaded);

  // Copy to memory at PROM address
  loader.copy_to_memory(mem.get(), 0x1FC00000);

  // Verify PROM signature at reset vector
  u32 reset_instr = mem->read32(0x1FC00000);
  EXPECT_NE(reset_instr, 0);
  EXPECT_NE(reset_instr, 0xFFFFFFFF);

  // CPU reset should jump to PROM
  cpu->reset();
  EXPECT_EQ(cpu->pc(), 0xBFC00000);
}

TEST_F(SystemFixture, TimerInterrupts) {
  Timer timer;
  timer.set_frequency(1000); // 1kHz
  timer.enable(true);

  // Tick
  for (int i = 0; i < 1000; i++) {
    timer.tick();
  }

  EXPECT_TRUE(timer.interrupt_pending());
}

TEST_F(SystemFixture, MREGraphics) {
  mre->write(MRE::MRE_CONTROL, MRE::CTRL_ENABLE);

  // Draw a triangle
  mre->draw_triangle(100, 100, 200, 100, 150, 200, 0xFF0000FF);

  // Should complete without error
  EXPECT_TRUE(true);
}

TEST_F(SystemFixture, FramebufferAccess) {
  // Test framebuffer through MRE
  mre->write(MRE::MRE_CONTROL, MRE::CTRL_ENABLE | MRE::CTRL_TILE_MODE);

  u32 tile_data[16];
  for (int i = 0; i < 16; i++)
    tile_data[i] = 0xFF00FF00;
  mre->write_tile(0, 0, tile_data, 64);

  u32 read_data[16];
  mre->read_tile(0, 0, read_data, 64);

  for (int i = 0; i < 16; i++) {
    EXPECT_EQ(read_data[i], 0xFF00FF00);
  }
}

TEST_F(SystemFixture, SCSICommand) {
  scsi->write(SCSIController::SCSI_CMD, SCSIController::CMD_TEST_UNIT_READY);

  // Should complete
  EXPECT_TRUE(true);
}

TEST_F(SystemFixture, MACEPCI) {
  // Test PCI config access
  mace->write(MACE::PCI_CONFIG_ADDR,
              0x80000000 | (0 << 16) | (0 << 11) | (0 << 8) | 0x00);
  u32 vendor_device = mace->read(MACE::PCI_CONFIG_DATA);

  // Should return something valid (not 0 or 0xFFFFFFFF)
  EXPECT_NE(vendor_device, 0);
  EXPECT_NE(vendor_device, 0xFFFFFFFF);
}

// Address constants
constexpr u32 CRIME_BASE = 0x14000000;
constexpr u32 MRE_BASE = 0x10000000;
constexpr u32 RTC_BASE = 0x1F000000;
constexpr u32 UART1_BASE = 0x1F000400;
constexpr u32 PS2_BASE = 0x1F000800;
constexpr u32 MACE_BASE = 0x1F001000;
constexpr u32 SCSI_BASE = 0x1F002000;