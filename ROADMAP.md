# O2Emu Roadmap

A from-scratch emulator for the **SGI O2 (IP32 / "Moosehead")** workstation.

**License:** BSD 3-Clause (see [LICENSE](LICENSE)).

## Guiding principle

> **Phase 2 rule:** be as accurate to the hardware as the documentation and
> specs allow. Every register, address, and behavior must be backed by a
> source (Linux/NetBSD driver, leaked IRIX source, or datasheet). No guessing.

---

## Phase 1 — Research (current)

**Goal:** exhaustive, well-sourced hardware documentation. **No emulator code.**

- [x] System architecture (`docs/architecture.md`)
- [x] CPU & memory (`docs/cpu-memory.md`)
- [x] Graphics — CRM/ICE/MRE/Display (`docs/graphics.md`)
- [x] I/O — MACE, PCI, SCSI, Ethernet (`docs/io.md`)
- [x] Register maps from drivers (`docs/register-maps.md`)
- [x] PROM firmware (`docs/prom.md`)
- [x] Research sources (`docs/sources.md`)
- [x] **Decompiled the PROM** into MIPS assembly
      (`samples/decompiled-prom/`) — recovered the full IP32 address map and
      CRIME/MACE/UART/RTC register maps from `definitions.h`
- [x] Collect and link datasheets (MIPS R5000/R10000/R12000, AIC-7880, SDRAM)
      — CPU datasheets collected under `docs/datasheets/CPU/`; CPU architecture
      documented in `docs/cpu-memory.md`
- [ ] Fill remaining open questions in each doc (MRE/ICE/Display register maps,
      MACE Ethernet/audio registers, DIMM SPD)

**Exit criteria:** every hardware subsystem has a sourced register map and the
memory map is fully documented.

> **Key asset:** the decompiled PROM (`samples/decompiled-prom/`) is the
> authoritative reference for the address map and register maps. It also gives
> us the exact POST boot sequence, default environment, and firmware layout —
> all needed to boot the emulator in Phase 2.

---

## Phase 2 — Emulation (C++)

**Goal:** a full-fledged C++ emulator, as accurate to the hardware as the
documentation and specs allow. **This is the core implementation phase.**

### 2.1 Core infrastructure
- [ ] Project scaffolding (CMake, C++20, cross-platform build)
- [ ] Memory map / address space abstraction
- [ ] Logging, tracing, and debug infrastructure
- [ ] Test harness (unit + integration)

### 2.2 CPU
- [ ] MIPS R5000 core (integer, FPU, MMU/TLB, caches)
- [ ] R10000/R12000 support (superscalar semantics, as needed)
- [ ] Exception / interrupt handling
- [ ] CP0 coprocessor (config, status, cause, EPC, etc.)

### 2.3 Memory & system controller
- [ ] CRIME (memory controller, ECC, refresh, interrupt controller)
- [ ] MRE (memory & rendering engine)
- [ ] Physical memory map, bank sizing, ECC behavior

### 2.4 Graphics (CRM chipset)
- [ ] Microprocessor (display list / vertex processing)
- [ ] ICE (imaging & compression engine)
- [ ] MRE rasterization + texture mapping
- [ ] Display Engine (analog video out)
- [ ] Framebuffer (tile-based GBE) + video modes

### 2.5 I/O (MACE)
- [ ] MACE ASIC (PCI bridge, ISA, PS/2, Ethernet, audio)
- [ ] PCI bus + single expansion slot
- [ ] SCSI (AIC-7880)
- [ ] Super I/O (serial + parallel)
- [ ] PS/2 keyboard + mouse
- [ ] 10/100 Ethernet

### 2.6 Firmware & boot
- [ ] PROM image loading (5-section SHDR layout, embedded ELF header)
- [ ] PROM execution at reset vector `0xBFC00000`
- [ ] Boot to IRIX and/or a hobbyist OS

> The decompiled PROM (`samples/decompiled-prom/`) gives us the exact POST boot
> sequence, subsection copy/checksum logic, TLB init, and default environment —
> a reference for validating the emulator's boot path.

### 2.7 GUI
- [ ] Windowed framebuffer display
- [ ] Keyboard/mouse input
- [ ] Debugger / inspector (registers, memory, disassembly)

**Exit criteria:** the emulator boots the PROM and reaches a usable OS prompt
with accurate hardware behavior.

---

## Phase 3 — Performance & polish

**Goal:** make the emulator fast, polished, and cross-platform.

- [ ] JIT compilation (dynamic recompilation of MIPS code)
- [ ] Performance optimizations (caching, threading, SIMD)
- [ ] GUI improvements (scaling, filters, save states, config UI)
- [ ] Cross-platform support (Windows, macOS, Linux)
- [ ] Full-featured emulator features (save/load state, cheats, netplay,
      controller mapping, audio output)
- [ ] Packaging and distribution

**Exit criteria:** a fast, polished, cross-platform emulator with a complete
feature set.

---

## Milestones

| Milestone | Phase | Definition of done |
|-----------|-------|--------------------|
| M1 | 1 | All docs complete and sourced; datasheets linked |
| M2 | 2 | CPU + memory execute the PROM |
| M3 | 2 | Graphics + I/O functional; OS boots |
| M4 | 2 | GUI complete; usable emulator |
| M5 | 3 | JIT + optimizations; cross-platform release |