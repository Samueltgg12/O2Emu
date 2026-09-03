/**
 * @file test_cpu.cpp
 * @brief CPU unit tests
 */

#include <gtest/gtest.h>
#include <o2emu/cpu/cp0.h>
#include <o2emu/cpu/mips_r5000.h>
#include <o2emu/memory/memory.h>
#include <o2emu/system/bus.h>

using namespace o2emu;
using namespace o2emu::cpu;
using namespace o2emu::memory;
using namespace o2emu::system;

class CPUMemory : public Memory {
public:
  CPUMemory() : Memory(0x1000000) {} // 16MB
};

class CPUBus : public Bus {
public:
  CPUBus() : Bus() {
    auto mem = std::make_shared<CPUMemory>();
    map_device(0x00000000, 0x1000000, mem);
  }
};

TEST(MIPSR5000, BasicInitialization) {
  auto bus = std::make_shared<CPUBus>();
  MIPSR5000 cpu(bus);

  // Reset should set PC to reset vector
  cpu.reset();
  EXPECT_EQ(cpu.pc(), 0xBFC00000);

  // General registers should be zero
  for (int i = 0; i < 32; i++) {
    EXPECT_EQ(cpu.gpr(i), 0);
  }
}

TEST(MIPSR5000, RegisterReadWrite) {
  auto bus = std::make_shared<CPUBus>();
  MIPSR5000 cpu(bus);
  cpu.reset();

  // Write to GPR
  cpu.set_gpr(1, 0x12345678);
  EXPECT_EQ(cpu.gpr(1), 0x12345678);

  // GPR 0 should always be zero
  cpu.set_gpr(0, 0xFFFFFFFF);
  EXPECT_EQ(cpu.gpr(0), 0);
}

TEST(MIPSR5000, BasicArithmetic) {
  auto bus = std::make_shared<CPUBus>();
  MIPSR5000 cpu(bus);
  cpu.reset();

  // ADDI: $1 = $0 + 10
  cpu.set_gpr(0, 0);
  cpu.execute_instruction(0x2001000A); // ADDI $1, $0, 10
  EXPECT_EQ(cpu.gpr(1), 10);

  // ADDIU: $2 = $1 + 20
  cpu.execute_instruction(0x24020014); // ADDIU $2, $1, 20
  EXPECT_EQ(cpu.gpr(2), 30);

  // ADD: $3 = $1 + $2
  cpu.execute_instruction(0x00221820); // ADD $3, $1, $2
  EXPECT_EQ(cpu.gpr(3), 40);
}

TEST(MIPSR5000, BranchInstructions) {
  auto bus = std::make_shared<CPUBus>();
  MIPSR5000 cpu(bus);
  cpu.reset();

  cpu.set_gpr(1, 10);
  cpu.set_gpr(2, 20);

  // BEQ: branch if equal (not taken)
  cpu.execute_instruction(0x10220001); // BEQ $1, $2, label
  EXPECT_EQ(cpu.pc(), 0xBFC00004);     // PC advanced normally

  // BEQ: branch if equal (taken)
  cpu.set_gpr(1, 10);
  cpu.set_gpr(2, 10);
  cpu.execute_instruction(0x10220001); // BEQ $1, $2, label
  EXPECT_EQ(cpu.pc(), 0xBFC0000C);     // PC = PC + 4 + (1 * 4)
}

TEST(MIPSR5000, JumpInstructions) {
  auto bus = std::make_shared<CPUBus>();
  MIPSR5000 cpu(bus);
  cpu.reset();

  // J: jump to address
  cpu.execute_instruction(0x08000000); // J 0x00000000
  EXPECT_EQ(cpu.pc(), 0x00000000);

  // JAL: jump and link
  cpu.execute_instruction(0x0C000001); // JAL 0x00000004
  EXPECT_EQ(cpu.pc(), 0x00000004);
  EXPECT_EQ(cpu.gpr(31), 0xBFC0000C); // RA = PC + 8
}

TEST(MIPSR5000, LoadStoreInstructions) {
  auto bus = std::make_shared<CPUBus>();
  MIPSR5000 cpu(bus);
  cpu.reset();

  // Write test data to memory
  bus->write32(0x1000, 0xDEADBEEF);

  // LW: load word
  cpu.execute_instruction(0x8C011000); // LW $1, 0x1000($0)
  EXPECT_EQ(cpu.gpr(1), 0xDEADBEEF);

  // SW: store word
  cpu.set_gpr(2, 0xCAFEBABE);
  cpu.execute_instruction(0xAC021004); // SW $2, 0x1004($0)
  EXPECT_EQ(bus->read32(0x1004), 0xCAFEBABE);
}

TEST(MIPSR5000, CP0Registers) {
  auto bus = std::make_shared<CPUBus>();
  MIPSR5000 cpu(bus);
  cpu.reset();

  // Read CP0 Status register
  u32 status = cpu.cp0().read_register(CP0::Register::STATUS);
  EXPECT_NE(status, 0);

  // Write CP0 Status register
  cpu.cp0().write_register(CP0::Register::STATUS, 0x0000FF03);
  EXPECT_EQ(cpu.cp0().read_register(CP0::Register::STATUS), 0x0000FF03);
}

TEST(MIPSR5000, ExceptionHandling) {
  auto bus = std::make_shared<CPUBus>();
  MIPSR5000 cpu(bus);
  cpu.reset();

  // Trigger address error exception (misaligned load)
  cpu.execute_instruction(0x8C010001); // LW $1, 1($0) - misaligned

  // Should have taken exception
  EXPECT_EQ(cpu.cp0().read_register(CP0::Register::CAUSE) & 0x7C,
            CP0::ExcCode::ADEL << 2);
}

TEST(MIPSR5000, TLBOperations) {
  auto bus = std::make_shared<CPUBus>();
  MIPSR5000 cpu(bus);
  cpu.reset();

  // Write TLB entry
  cpu.cp0().write_register(CP0::Register::ENTRYHI, 0x80000000);
  cpu.cp0().write_register(CP0::Register::ENTRYLO0,
                           0x00000003); // Valid, Dirty, Global
  cpu.cp0().write_register(CP0::Register::ENTRYLO1, 0x00000003);
  cpu.cp0().write_register(CP0::Register::INDEX, 0);
  cpu.execute_instruction(0x42000008); // TLBWI

  // Read back
  cpu.cp0().write_register(CP0::Register::INDEX, 0);
  cpu.execute_instruction(0x42000001); // TLBR

  EXPECT_EQ(cpu.cp0().read_register(CP0::Register::ENTRYHI), 0x80000000);
  EXPECT_EQ(cpu.cp0().read_register(CP0::Register::ENTRYLO0), 0x00000003);
}