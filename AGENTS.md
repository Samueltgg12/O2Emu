# AGENTS.md

Guidance for AI coding agents working in this repository.

## Project overview

This is an emulator for the SGI O2 (IP32) workstation. **Phase 1 (Research)
is complete** — every hardware subsystem has a sourced register map under
`docs/`. We are now in **Phase 2 (Emulation)**: a full-fledged C++ emulator,
as accurate to the hardware as the documentation and specs allow.

## Phases

- **Phase 1 — Research ✅ complete:** exhaustive, well-sourced hardware
  documentation under `docs/`. All register maps sourced.
- **Phase 2 — Emulation (current):** a full-fledged **C++** emulator, as
  accurate to the hardware as the documentation and specs allow. CPU, memory,
  graphics, I/O, and the PROM firmware, with a GUI.
- **Phase 3 — Performance & polish:** JIT compilation, optimizations, GUI
  improvements, cross-platform support, and full-featured emulator features.

See `ROADMAP.md` for the full plan.

## Repository layout

```
README.md            # GitHub-facing overview
AGENTS.md            # This file
CLAUDE.md            # Claude-specific guidanceROADMAP.md           # Full project plan (phases 1-3)docs/                # All research documentation (the current deliverable)
  README.md          #   docs index
  architecture.md    #   system architecture
  cpu-memory.md      #   CPU & memory
  graphics.md        #   CRM/ICE/MRE/Display
  io.md              #   MACE, PCI, SCSI, Ethernet
  register-maps.md   #   register maps from drivers
  prom.md            #   IP32 PROM firmware
  sources.md         #   research sources
samples/             # firmware images
  ip32prom.rev4.18.bin
  ip32prom.rev4.3.bin
```

## Conventions

- **Accuracy is the product.** Every emulated register, address, and behavior
  must be backed by a source in `docs/` (ASIC spec, driver, header, or leaked
  IRIX source). Do not invent register names, addresses, or behaviors.
- **Every hardware claim must be sourced.** Cite the driver file, header, or
  leaked IRIX source that backs each register/address/behavior.
- **Keep docs concise.** A few lines per section, not essays. Use tables for
  register maps and address layouts.
- **Update the index.** When adding a doc, add a row to `docs/README.md`.
- **Update `docs/sources.md`** when you discover a new source (repo, driver,
  datasheet, forum thread).
- **Preserve the PROM checksum.** The IP32 PROM uses a two's complement
  checksum; never modify a firmware image without recomputing it.

## Key facts to remember

- O2 = SGI IP32, codename "Moosehead".
- UMA: CPU, graphics, and I/O share main memory over a 133 MHz 144-bit bus.
- Graphics = CRM chipset (Microprocessor + ICE, MRE, Display ASICs).
- I/O = MACE ASIC (PCI, ISA, PS/2, Ethernet).
- PROM reset vector: `0xBFC00000`; firmware VMA `0x81000000`.
- PROM images in `samples/`; rev4.18 MD5 `c9725e036052cf1f3e6258eb9bc687fa`.

## License

BSD 3-Clause License. See [LICENSE](LICENSE).

## Research sources

- Linux: `arch/mips/sgi-ip32/`, `arch/mips/include/asm/ip32/`, `drivers/video/crmfb.c`
- NetBSD: `sys/arch/sgimips/`
- Leaked IRIX source: `calmsacibis995/irix-657m-src` (6.5.7m) and
  `jacklin9/IRIX-6.5.17-Src` (6.5.17) — includes the actual IP32 PROM source
  under `stand/arcs/`
- `mattst88/ip32prom-decompiler` — Rust PROM decompiler

## When in doubt

Phase 1 (research) is complete; Phase 2 (emulation) is in scope. When hardware
behavior is unclear, consult the ASIC specs in `docs/manuals-specs/` first,
then the driver sources. Ask the user before deviating from documented
behavior.

<!-- AICB:BEGIN {"version":1,"target":"AGENTS.md","generatedAt":"2026-09-05T21:52:04.802Z","hash":"sha256:aed970dedd2f894fbecf12f12e1087d23f3f8843893c5011f17bf9d8242947ac"} -->
# AI Context Bridge — Handoff

Workspace: `/home/samuel/SGI Projects/O2Emu`

## Spec / context files (read these first)
- `docs/README.md` _(spec)_ — Project README (docs)
- `README.md` _(spec)_ — Project README
- `AGENTS.md` _(spec)_ — Multi-agent instructions
- `ROADMAP.md` _(spec)_ — Spec: ROADMAP.md
- `.agent/AGENTS.md` _(spec)_ — Multi-agent instructions (.agent)
- `.cursorrules` _(spec)_ — Cursor rules
- `.windsurfrules` _(spec)_ — Windsurf rules
- `.github/copilot-instructions.md` _(spec)_ — GitHub Copilot instructions (.github)
- `samples/irixsrc/irix-657m-src/README.md` _(spec)_ — Project README (samples/irixsrc/irix-657m-src)
- `samples/netbsd/usr (2)/usr/src/usr.bin/xlint/lint1/README.md` _(spec)_ — Project README (samples/netbsd/usr (2)/usr/src/usr.bin/xlint/lint1)
- `samples/netbsd/usr (2)/usr/src/external/public-domain/sqlite/sqlite2mdoc/README.md` _(spec)_ — Project README (samples/netbsd/usr (2)/usr/src/external/public-domain/sqlite/sqlite2mdoc)
- `samples/netbsd/usr (2)/usr/src/usr.bin/indent/README.md` _(spec)_ — Project README (samples/netbsd/usr (2)/usr/src/usr.bin/indent)
- `samples/netbsd/usr (2)/usr/src/README.md` _(spec)_ — Project README (samples/netbsd/usr (2)/usr/src)
- `samples/linux/tools/sched_ext/README.md` _(spec)_ — Project README (samples/linux/tools/sched_ext)
- `samples/linux/rust/zerocopy/README.md` _(spec)_ — Project README (samples/linux/rust/zerocopy)
- `samples/linux/rust/zerocopy-derive/README.md` _(spec)_ — Project README (samples/linux/rust/zerocopy-derive)
- `samples/linux/rust/syn/README.md` _(spec)_ — Project README (samples/linux/rust/syn)
- `samples/linux/rust/quote/README.md` _(spec)_ — Project README (samples/linux/rust/quote)
- `samples/linux/rust/proc-macro2/README.md` _(spec)_ — Project README (samples/linux/rust/proc-macro2)
- `samples/linux/rust/pin-init/README.md` _(spec)_ — Project README (samples/linux/rust/pin-init)
- `samples/irixsrc/IRIX-6.5.17-Src/README.md` _(spec)_ — Project README (samples/irixsrc/IRIX-6.5.17-Src)
- `samples/linux/drivers/gpu/drm/amd/display/dc/dml2_0/README.md` _(spec)_ — Project README (samples/linux/drivers/gpu/drm/amd/display/dc/dml2_0)
- `CLAUDE.md` _(spec)_ — Claude Code instructions
- `GEMINI.md` _(spec)_ — Gemini instructions
- `AGENT.md` _(spec)_ — Multi-agent instructions

## Working files (current focus)
- `emu/CMakeLists.txt` _(auto:dwell)_
- `emu/include/o2emu/devices/scsicontroller.h` _(auto:recent-edit)_
- `emu/src/devices/ps2.cpp` _(auto:recent-edit)_
- `emu/src/devices/uart.cpp` _(auto:recent-edit)_
- `emu/src/devices/scsicontroller.cpp` _(auto:recent-edit)_
- `gui/CMakeLists.txt` _(auto:recent-edit)_
- `.github/workflows/cmake-multi-platform.yml` _(auto:recent-edit)_
- `emu/include/o2emu/firmware/prom.h` _(auto:recent-edit)_
- `emu/src/firmware/prom.cpp` _(auto:recent-edit)_

## How to use this handoff
1. Read every file under "Spec / context files" before acting.
2. Continue the work described in the most recent thought.
3. Honor skill statuses: `ENABLED` use freely, `ASK` require explicit user confirmation each time, `DISABLED` must not be used.
4. When you reach a non-trivial decision, append a thought to `.aicb/state.json` (modelId + text + sourceReference if relevant).
<!-- AICB:END -->
