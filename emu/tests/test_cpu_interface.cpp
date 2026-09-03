#include "o2emu/bus.h"
#include "o2emu/cpu_interface.h"
#include <cassert>
#include <cstdint>
#include <string>
#include <unordered_map>

class TestRam : public o2emu::BusDevice {
public:
  explicit TestRam(uint32_t base = 0xbfc00000u, uint32_t size = 0x1000u)
      : base_(base), size_(size) {}

  uint32_t base_address() const override { return base_; }
  uint32_t size() const override { return size_; }
  std::string name() const override { return "test_ram"; }

  uint8_t read8(uint32_t offset) override {
    const uint32_t value = read32(offset & ~3u);
    const uint32_t shift = (offset & 3u) * 8u;
    return static_cast<uint8_t>((value >> shift) & 0xffu);
  }

  uint16_t read16(uint32_t offset) override {
    const uint32_t value = read32(offset & ~1u);
    const uint32_t shift = (offset & 1u) * 8u;
    return static_cast<uint16_t>((value >> shift) & 0xffffu);
  }

  uint32_t read32(uint32_t offset) override {
    if (offset >= size_) {
      return 0;
    }
    return memory_[offset];
  }

  uint64_t read64(uint32_t offset) override {
    return (static_cast<uint64_t>(read32(offset)) |
            (static_cast<uint64_t>(read32(offset + 4)) << 32));
  }

  void write8(uint32_t offset, uint8_t val) override {
    uint32_t current = read32(offset & ~3u);
    const uint32_t shift = (offset & 3u) * 8u;
    current &= ~(0xffu << shift);
    current |= (static_cast<uint32_t>(val) << shift);
    memory_[offset & ~3u] = current;
  }

  void write16(uint32_t offset, uint16_t val) override {
    uint32_t current = read32(offset & ~1u);
    const uint32_t shift = (offset & 1u) * 8u;
    current &= ~(0xffffu << shift);
    current |= (static_cast<uint32_t>(val) << shift);
    memory_[offset & ~1u] = current;
  }

  void write32(uint32_t offset, uint32_t val) override {
    if (offset < size_) {
      memory_[offset] = val;
    }
  }

  void write64(uint32_t offset, uint64_t val) override {
    write32(offset, static_cast<uint32_t>(val));
    write32(offset + 4, static_cast<uint32_t>(val >> 32));
  }

private:
  uint32_t base_;
  uint32_t size_;
  std::unordered_map<uint32_t, uint32_t> memory_;
};

int main() {
  auto cpu = o2emu::create_cpu("interpreter");
  assert(cpu != nullptr);

  o2emu::SystemBus bus;
  TestRam ram(0xbfc00000u, 0x1000u);
  bus.add_device(std::make_unique<TestRam>(ram));
  cpu->init(&bus);
  cpu->reset();

  assert(cpu->state().pc == 0xbfc00000u);
  assert(cpu->state().next_pc == 0xbfc00004u);
  assert(cpu->state().cp0.prid == 0x00002300u);
  assert(cpu->has_breakpoint(0) == false);

  cpu->add_breakpoint(0x1234);
  assert(cpu->has_breakpoint(0x1234));

  // ADDIU $v0, $at, 5 uses opcode 0x24, rs=1, rt=2, imm=5.
  bus.write32(0xbfc00000u, 0x24420005u);
  cpu->mutable_state().gpr[1] = 7u;
  cpu->step();
  assert(cpu->state().gpr[2] == 12u);
  assert(cpu->state().pc == 0xbfc00004u);

  cpu->reset();
  cpu->mutable_state().cp0.status = 0x00000001u;
  cpu->raise_interrupt(2u);
  assert(cpu->state().cp0.cause & (1u << 10u));
  assert(cpu->state().exception_pending == true);

  cpu->reset();
  cpu->mutable_state().cp0.status = 0x00000001u;
  cpu->raise_interrupt(0u);
  assert(cpu->state().cp0.cause & (1u << 8u));
  assert(cpu->state().exception_pending == true);
  assert(cpu->state().exception_code == 0u);

  auto ram_map = o2emu::create_ram_map(0x2000u);
  ram_map->write32(0x100, 0x12345678u);
  assert(ram_map->read32(0x100) == 0x12345678u);
  assert(ram_map->read8(0x100) == 0x78u);

  // MMU/TLB scaffolding: a mapped VPN should translate to the physical page.
  o2emu::TlbEntry tlb_entry{};
  tlb_entry.valid = true;
  tlb_entry.asid = 0;
  tlb_entry.vpn = 0x00001000u;
  tlb_entry.page_mask = 0x00000fffu;
  tlb_entry.pfn = 0x00002000u;
  cpu->add_tlb_entry(tlb_entry);
  assert(cpu->translate_address(0x00001004u) == 0x00002004u);
  assert(cpu->state().cache.icache_enabled == true);

  cpu->set_cache_mode(true, false);
  assert(cpu->state().cache.icache_enabled == true);
  assert(cpu->state().cache.dcache_enabled == false);

  // Full decoder coverage: PROM/IRIX code uses delay-slot jumps and wide loads.
  cpu->reset(0x80000000u);
  bus.write32(0x80000000u, 0x3c081234u); // lui $t0, 0x1234
  cpu->step();
  assert(cpu->state().gpr[8] == 0x12340000u);

  cpu->reset(0x80000000u);
  bus.write32(0x80000000u, 0x0c000400u); // jal 0x00001000
  cpu->step();
  assert(cpu->state().gpr[31] == 0x80000008u);
  assert(cpu->state().pc == 0x80000004u);
  assert(cpu->state().next_pc == 0x80001000u);
  assert(cpu->state().in_delay_slot == true);

  return 0;
}
