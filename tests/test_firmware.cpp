/**
 * @file test_firmware.cpp
 * @brief Firmware/PROM unit tests
 */

#include <gtest/gtest.h>
#include <o2emu/firmware/prom.h>
#include <o2emu/firmware/prom_loader.h>
#include <o2emu/memory/memory.h>
#include <o2emu/system/bus.h>
#include <vector>

using namespace o2emu;
using namespace o2emu::firmware;
using namespace o2emu::memory;
using namespace o2emu::system;

TEST(PromLoader, LoadRev418) {
  PromLoader loader;

  // Load the rev4.18 PROM
  bool result = loader.load("samples/ip32prom.rev4.18.bin");
  EXPECT_TRUE(result);

  // Check sections
  EXPECT_EQ(loader.sections().size(), 5);

  // Check entry point
  EXPECT_EQ(loader.entry_point(), 0x81000000);
}

TEST(PromLoader, LoadRev43) {
  PromLoader loader;

  bool result = loader.load("samples/ip32prom.rev4.3.bin");
  EXPECT_TRUE(result);

  EXPECT_EQ(loader.sections().size(), 5);
  EXPECT_EQ(loader.entry_point(), 0x81000000);
}

TEST(PromLoader, SectionHeaders) {
  PromLoader loader;
  loader.load("samples/ip32prom.rev4.18.bin");

  const auto &sections = loader.sections();

  // Section 0: SLOADER (bootstrap)
  EXPECT_EQ(sections[0].name, "SLOADER");
  EXPECT_EQ(sections[0].vma, 0x81000000);
  EXPECT_GT(sections[0].size, 0);

  // Section 1: POST1 (power-on self test)
  EXPECT_EQ(sections[1].name, "POST1");
  EXPECT_GT(sections[1].size, 0);

  // Section 2: FIRMWARE (main firmware)
  EXPECT_EQ(sections[2].name, "FIRMWARE");
  EXPECT_GT(sections[2].size, 0);

  // Section 3: TRAILING
  EXPECT_EQ(sections[3].name, "TRAILING");

  // Section 4: VERSION
  EXPECT_EQ(sections[4].name, "VERSION");
}

TEST(PromLoader, Checksum) {
  PromLoader loader;
  loader.load("samples/ip32prom.rev4.18.bin");

  // Verify checksum
  EXPECT_TRUE(loader.verify_checksum());
}

TEST(PromLoader, InvalidFile) {
  PromLoader loader;

  bool result = loader.load("nonexistent.bin");
  EXPECT_FALSE(result);
}

TEST(Prom, BasicOperation) {
  auto bus = std::make_shared<Bus>();
  auto mem = std::make_shared<Memory>(0x1000000);
  bus->map_device(0x00000000, 0x1000000, mem);

  Prom prom(bus);

  // Load PROM into memory
  PromLoader loader;
  loader.load("samples/ip32prom.rev4.18.bin");
  loader.copy_to_memory(mem.get(), 0x1FC00000); // PROM physical address

  // Reset should jump to PROM entry
  prom.reset();

  // PC should be at reset vector
  // (This depends on CPU integration)
}

TEST(Prom, Sections) {
  Prom prom;

  // Test section parsing
  std::vector<u8> data(0x100000, 0);
  // Fill with dummy SHDR sections

  // This would test the internal section parsing
  // For now just verify the class exists
  EXPECT_TRUE(true);
}

TEST(Prom, ELFHeader) {
  PromLoader loader;
  loader.load("samples/ip32prom.rev4.18.bin");

  // The PROM has an embedded ELF header in the FIRMWARE section
  const auto &sections = loader.sections();
  const auto &firmware = sections[2];

  // Check for ELF magic
  EXPECT_GE(firmware.data.size(), 16);
  // ELF magic is at offset 0 of firmware section
  // But it's embedded, so we'd need to parse it
}