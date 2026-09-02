# AGENTS.md

Guidance for AI coding agents working in this repository.

## Project overview

This is a **research-first** project: an emulator for the SGI O2 (IP32)
workstation. We are currently in **Phase 1 (Research)** — there is **no
emulator code yet**. The deliverable right now is accurate, well-sourced
hardware documentation.

## Repository layout

```
README.md            # GitHub-facing overview
AGENTS.md            # This file
CLAUDE.md            # Claude-specific guidance
docs/                # All research documentation (the current deliverable)
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

- **Documentation is the product.** When asked to "implement" something, in
  this phase that means writing/updating docs under `docs/`, not writing code.
- **Every hardware claim must be sourced.** Cite the driver file, header, or
  leaked IRIX source that backs each register/address/behavior. Do not invent
  register names or addresses.
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

## Research sources

- Linux: `arch/mips/sgi-ip32/`, `arch/mips/include/asm/ip32/`, `drivers/video/crmfb.c`
- NetBSD: `sys/arch/sgimips/`
- Leaked IRIX source: `calmsacibis995/irix-657m-src` (6.5.7m) and
  `jacklin9/IRIX-6.5.17-Src` (6.5.17) — includes the actual IP32 PROM source
  under `stand/arcs/`
- `mattst88/ip32prom-decompiler` — Rust PROM decompiler

## When in doubt

Ask the user before writing emulator code. This is a research phase; premature
implementation is out of scope.