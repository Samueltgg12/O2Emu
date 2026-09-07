/**
 * @file main.cpp
 * @brief CLI entry point for O2Emu
 */

#include <cstdlib>
#include <iostream>
#include <o2emu/cpu/cpu.h>
#include <o2emu/devices/mace/mace.h>
#include <o2emu/devices/ps2.h>
#include <o2emu/devices/rtc.h>
#include <o2emu/devices/scsicontroller.h>
#include <o2emu/devices/uart.h>
#include <o2emu/firmware/prom_loader.h>
#include <o2emu/graphics/gbe_framebuffer.h>
#include <o2emu/graphics/ice.h>
#include <o2emu/logging/logger.h>
#include <o2emu/memory/crime_device.h>
#include <o2emu/memory/memory.h>
#include <o2emu/memory/mre_device.h>
#include <o2emu/o2emu.h>
#include <o2emu/system/bus.h>
#include <string>

using namespace o2emu;

void print_usage(const char *prog) {
  std::cout << "O2Emu - SGI O2 (IP32) Emulator v" << VERSION << "\n";
  std::cout << "Usage: " << prog << " [options]\n\n";
  std::cout << "Options:\n";
  std::cout << "  -p, --prom <file>     PROM image file (default: "
               "samples/ip32prom.rev4.18.bin)\n";
  std::cout
      << "  -m, --memory <MB>     RAM size in MB (default: 256, max: 1024)\n";
  std::cout << "  -c, --cycles <N>      Number of cycles to execute (default: "
               "run forever)\n";
  std::cout << "  -d, --debug           Enable debug logging\n";
  std::cout << "  -t, --trace           Enable trace logging\n";
  std::cout << "  -l, --log <file>      Log to file\n";
  std::cout << "  -h, --help            Show this help\n";
  std::cout << "\n";
  std::cout << "Examples:\n";
  std::cout << "  " << prog << " -p samples/ip32prom.rev4.18.bin -m 256\n";
  std::cout << "  " << prog
            << " -p samples/ip32prom.rev4.18.bin -c 1000000 -d\n";
}

int main(int argc, char *argv[]) {
  // Default options
  std::string prom_path = "samples/ip32prom.rev4.18.bin";
  u32 ram_mb = 256;
  u64 max_cycles = 0; // 0 = run forever
  logging::Level log_level = logging::Level::INFO;
  std::string log_file;

  // Parse arguments
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    if (arg == "-h" || arg == "--help") {
      print_usage(argv[0]);
      return 0;
    } else if (arg == "-p" || arg == "--prom") {
      if (i + 1 < argc) {
        prom_path = argv[++i];
      } else {
        std::cerr << "Error: --prom requires a file path\n";
        return 1;
      }
    } else if (arg == "-m" || arg == "--memory") {
      if (i + 1 < argc) {
        ram_mb = std::stoul(argv[++i]);
        if (ram_mb > 1024)
          ram_mb = 1024;
        if (ram_mb < 1)
          ram_mb = 1;
      } else {
        std::cerr << "Error: --memory requires a size in MB\n";
        return 1;
      }
    } else if (arg == "-c" || arg == "--cycles") {
      if (i + 1 < argc) {
        max_cycles = std::stoull(argv[++i]);
      } else {
        std::cerr << "Error: --cycles requires a number\n";
        return 1;
      }
    } else if (arg == "-d" || arg == "--debug") {
      log_level = logging::Level::DEBUG;
    } else if (arg == "-t" || arg == "--trace") {
      log_level = logging::Level::TRACE;
    } else if (arg == "-l" || arg == "--log") {
      if (i + 1 < argc) {
        log_file = argv[++i];
      } else {
        std::cerr << "Error: --log requires a file path\n";
        return 1;
      }
    } else {
      std::cerr << "Error: Unknown option: " << arg << "\n";
      print_usage(argv[0]);
      return 1;
    }
  }

  // Initialize logging
  auto &logger = logging::Logger::instance();
  logger.set_level(log_level);
  if (!log_file.empty()) {
    logger.set_output_file(log_file);
  }

  O2EMU_LOG_INFO_F("Starting O2Emu v%s", VERSION);
  O2EMU_LOG_INFO_F("PROM: %s", prom_path.c_str());
  O2EMU_LOG_INFO_F("RAM: %u MB", ram_mb);
  if (max_cycles > 0) {
    O2EMU_LOG_INFO_F("Max cycles: %llu", max_cycles);
  }

  try {
    // Initialize memory system
    memory::Memory memory;
    memory.init(ram_mb);

    // Initialize system bus
    system::Bus bus;

    // Initialize CPU
    cpu::CPU cpu;
    cpu.reset(ip32::PROM_RESET_VECTOR);

    // Connect CPU to memory via bus
    cpu.set_memory_read_callback([&bus](u32 addr, u32 size) -> u32 {
      switch (size) {
      case 1:
        return bus.read8(addr);
      case 2:
        return bus.read16(addr);
      case 4:
        return bus.read32(addr);
      default:
        return 0;
      }
    });

    cpu.set_memory_write_callback([&bus](u32 addr, u32 size, u32 value) {
      switch (size) {
      case 1:
        bus.write8(addr, value);
        break;
      case 2:
        bus.write16(addr, value);
        break;
      case 4:
        bus.write32(addr, value);
        break;
      }
    });

    // Attach memory to bus
    bus.attach_memory(&memory);

    // Attach system devices
    bus.attach_device(std::make_unique<memory::CRIMEDevice>(memory.crime()));
    bus.attach_device(std::make_unique<memory::MREDevice>(memory.mre()));
    bus.attach_device(std::make_unique<graphics::ICE>());
    bus.attach_device(std::make_unique<graphics::GBEFramebuffer>());
    bus.attach_device(std::make_unique<devices::MACE>(memory));
    bus.attach_device(std::make_unique<devices::PS2>(0x1F320000, 5, 6));

    auto uart1 = std::make_unique<devices::UART>(0x1F390000, 7);
    uart1->set_tx_callback([](u8 ch) {
      std::cout.put(static_cast<char>(ch));
      std::cout.flush();
    });
    bus.attach_device(std::move(uart1));
    bus.attach_device(std::make_unique<devices::UART>(0x1F398000, 8));
    bus.attach_device(std::make_unique<devices::RTC>(0x1F3A0000));
    bus.attach_device(std::make_unique<devices::SCSIController>());

    // Load PROM
    firmware::PROMLoader prom_loader(&bus, &cpu);
    if (!prom_loader.load_prom(prom_path)) {
      O2EMU_LOG_ERROR_F("Failed to load PROM from %s", prom_path.c_str());
      return 1;
    }

    O2EMU_LOG_INFO("PROM loaded successfully");
    O2EMU_LOG_INFO_F("Entry point: 0x%08X", prom_loader.prom().entry_point());

    // Execute PROM bootstrap (maps sections and initializes CPU)
    prom_loader.execute_bootstrap();

    // Run emulation
    O2EMU_LOG_INFO("Starting emulation...");

    if (max_cycles > 0) {
      cpu.run(max_cycles);
    } else {
      // Run until interrupted
      while (true) {
        cpu.step();

        // Periodic status
        if (cpu.cycles_executed() % 1000000 == 0) {
          O2EMU_LOG_DEBUG_F("Cycles: %llu, PC: 0x%08X", cpu.cycles_executed(),
                            cpu.state().pc);
        }
      }
    }

    O2EMU_LOG_INFO_F("Emulation stopped after %llu cycles",
                     cpu.cycles_executed());
    cpu.dump_registers();

  } catch (const std::exception &e) {
    O2EMU_LOG_FATAL_F("Exception: %s", e.what());
    return 1;
  }

  return 0;
}