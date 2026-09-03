#include "o2emu/bus.h"
#include "o2emu/cpu_interface.h"
#include "o2emu/logging.h"
#include <iostream>

int main() {
  o2emu::Logger::instance().set_level(o2emu::LogLevel::Info);

  o2emu::SystemBus bus;
  auto cpu = o2emu::create_cpu("interpreter");
  if (!cpu) {
    std::cerr << "Failed to create CPU\n";
    return 1;
  }

  cpu->init(&bus);
  cpu->reset();
  std::cout << "CPU initialized at PC 0x" << std::hex << cpu->state().pc
            << "\n";
  return 0;
}
