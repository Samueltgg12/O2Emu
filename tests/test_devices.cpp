/**
 * @file test_devices.cpp
 * @brief Device unit tests
 */

#include <gtest/gtest.h>
#include <o2emu/devices/device.h>
#include <o2emu/devices/mace/mace.h>
#include <o2emu/devices/ps2.h>
#include <o2emu/devices/rtc.h>
#include <o2emu/devices/scsicontroller.h>
#include <o2emu/devices/uart.h>
#include <o2emu/system/bus.h>

using namespace o2emu;
using namespace o2emu::devices;
using namespace o2emu::system;

TEST(RTC, BasicOperation) {
  RTC rtc;

  // Test register access
  rtc.write(RTC::RTC_SECONDS, 0x30); // 30 seconds
  EXPECT_EQ(rtc.read(RTC::RTC_SECONDS), 0x30);

  rtc.write(RTC::RTC_MINUTES, 0x45); // 45 minutes
  EXPECT_EQ(rtc.read(RTC::RTC_MINUTES), 0x45);

  rtc.write(RTC::RTC_HOURS, 0x12); // 12 hours
  EXPECT_EQ(rtc.read(RTC::RTC_HOURS), 0x12);
}

TEST(RTC, TimeUpdate) {
  RTC rtc;

  rtc.write(RTC::RTC_SECONDS, 0x59);
  rtc.write(RTC::RTC_MINUTES, 0x59);
  rtc.write(RTC::RTC_HOURS, 0x23);

  // Tick should roll over
  rtc.tick();
  EXPECT_EQ(rtc.read(RTC::RTC_SECONDS), 0x00);
  EXPECT_EQ(rtc.read(RTC::RTC_MINUTES), 0x00);
  EXPECT_EQ(rtc.read(RTC::RTC_HOURS), 0x00);
}

TEST(RTC, Alarm) {
  RTC rtc;

  rtc.write(RTC::RTC_ALARM_SECONDS, 0x30);
  rtc.write(RTC::RTC_ALARM_MINUTES, 0x45);
  rtc.write(RTC::RTC_ALARM_HOURS, 0x12);

  rtc.write(RTC::RTC_SECONDS, 0x29);
  rtc.write(RTC::RTC_MINUTES, 0x45);
  rtc.write(RTC::RTC_HOURS, 0x12);

  rtc.tick(); // Should trigger alarm
  EXPECT_TRUE(rtc.alarm_triggered());
}

TEST(UART, BasicOperation) {
  UART uart;

  // Test register access
  uart.write(UART::UART_THR, 'A');
  EXPECT_EQ(uart.read(UART::UART_RHR), 'A');

  // Test line control
  uart.write(UART::UART_LCR, 0x03); // 8N1
  EXPECT_EQ(uart.read(UART::UART_LCR), 0x03);
}

TEST(UART, FIFO) {
  UART uart;

  uart.write(UART::UART_FCR,
             UART::FCR_FIFO_ENABLE | UART::FCR_CLEAR_RX | UART::FCR_CLEAR_TX);

  // Fill TX FIFO
  for (int i = 0; i < 16; i++) {
    uart.write(UART::UART_THR, 'A' + i);
  }

  // Should be full
  EXPECT_TRUE(uart.tx_fifo_full());
}

TEST(UART, Interrupts) {
  UART uart;

  uart.write(UART::UART_IER, UART::IER_RX_DATA | UART::IER_TX_EMPTY);

  // Trigger RX interrupt
  uart.write(UART::UART_RHR, 'A');
  EXPECT_TRUE(uart.interrupt_pending());

  // Read should clear
  uart.read(UART::UART_RHR);
  EXPECT_FALSE(uart.interrupt_pending());
}

TEST(PS2, Keyboard) {
  PS2 ps2;

  // Send keyboard command
  ps2.write(PS2::PS2_DATA, 0xED); // Set LEDs
  ps2.write(PS2::PS2_DATA, 0x02); // Num Lock

  // Should ACK
  EXPECT_EQ(ps2.read(PS2::PS2_DATA), 0xFA);
}

TEST(PS2, Mouse) {
  PS2 ps2;

  // Enable mouse
  ps2.write(PS2::PS2_CMD, PS2::CMD_ENABLE_MOUSE);

  // Send mouse data
  ps2.inject_mouse_data(0x08, 10, -5); // Buttons, X, Y

  // Should generate interrupt
  EXPECT_TRUE(ps2.mouse_interrupt_pending());
}

TEST(MACE, PCI) {
  MACE mace;

  // Test PCI config space access
  mace.write(MACE::PCI_CONFIG_ADDR,
             0x80000000 | (0 << 16) | (0 << 11) | (0 << 8) | 0x00);
  u32 vendor_device = mace.read(MACE::PCI_CONFIG_DATA);
  EXPECT_NE(vendor_device, 0);
  EXPECT_NE(vendor_device, 0xFFFFFFFF);
}

TEST(MACE, ISA) {
  MACE mace;

  // Test ISA bus access
  mace.write(MACE::ISA_BASE + 0x3F8, 0x00); // COM1
  EXPECT_EQ(mace.read(MACE::ISA_BASE + 0x3F8), 0x00);
}

TEST(SCSIController, BasicOperation) {
  SCSIController scsi;

  // Test register access
  scsi.write(SCSIController::SCSI_DATA, 0x12);
  EXPECT_EQ(scsi.read(SCSIController::SCSI_DATA), 0x12);

  // Test command
  scsi.write(SCSIController::SCSI_CMD, SCSIController::CMD_RESET);
  EXPECT_TRUE(scsi.busy());
}

TEST(Device, BaseClass) {
  class TestDevice : public Device {
  public:
    TestDevice() : Device("TestDevice", 0x1000) {}
    u32 read(u32 offset) override { return regs_[offset / 4]; }
    void write(u32 offset, u32 value) override { regs_[offset / 4] = value; }
    void reset() override { std::fill(regs_.begin(), regs_.end(), 0); }

  private:
    std::array<u32, 256> regs_ = {};
  };

  TestDevice dev;
  dev.write(0x10, 0x12345678);
  EXPECT_EQ(dev.read(0x10), 0x12345678);
  dev.reset();
  EXPECT_EQ(dev.read(0x10), 0);
}