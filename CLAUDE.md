# CLAUDE.md

Project-specific guidance for Claude Code (and other AI assistants) working in
this repository.

## What this project is

An emulator for the **SGI O2 (IP32)** workstation, currently in **Phase 1:
Research**. There is **no emulator code yet** — the current deliverable is
accurate, well-sourced hardware documentation under `docs/`.

## Phases

- **Phase 1 — Research (current):** exhaustive, well-sourced hardware
  documentation under `docs/`. No emulator code.
- **Phase 2 — Emulation:** a full-fledged **C++** emulator, as accurate to the
  hardware as the documentation and specs allow. CPU, memory, graphics, I/O,
  and the PROM firmware, with a GUI.
- **Phase 3 — Performance & polish:** JIT compilation, optimizations, GUI
  improvements, cross-platform support, and full-featured emulator features.

See `ROADMAP.md` for the full plan.

## How to work here

1. **Start with `docs/README.md`** for the documentation index, then read the
   relevant module doc before answering or editing.
2. **Documentation is the product.** In this phase, "implementing" means
   writing/updating docs, not writing emulator code.
3. **Every hardware claim must be sourced.** Cite the Linux/NetBSD driver,
   header, or leaked IRIX source that backs each register, address, or
   behavior. Never invent register names or addresses.
4. **Keep docs concise.** Use tables for register maps and address layouts.
5. **Update the index and sources.** Add new docs to `docs/README.md` and new
   sources to `docs/sources.md`.

## Repository layout

```
README.md            # GitHub-facing overview
AGENTS.md            # AI agent guidance (generic)
CLAUDE.md            # This file (Claude-specific)ROADMAP.md           # Full project plan (phases 1-3)docs/                # All research documentation
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

## Key facts

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

## Scope guard

Do **not** write emulator code without explicit user approval. This is a
research phase; premature implementation is out of scope.

<!-- AICB:BEGIN {"version":1,"target":"CLAUDE.md","generatedAt":"2026-09-02T17:24:20.080Z","hash":"sha256:5d7fe99ceb386d64f83cfc03f7f600fb33116f3db089a6e77e59603e64586eed"} -->
# AI Context Bridge — Handoff

Workspace: `/home/samuel/SGI Projects/O2Emu`

## Spec / context files (read these first)
- `docs/README.md` _(spec)_ — Project README (docs)
- `README.md` _(spec)_ — Project README
- `AGENTS.md` _(spec)_ — Multi-agent instructions
- `ROADMAP.md` _(spec)_ — Spec: ROADMAP.md
- `CLAUDE.md` _(spec)_ — Claude Code instructions
- `.agent/AGENTS.md` _(spec)_ — Multi-agent instructions (.agent)
- `GEMINI.md` _(spec)_ — Gemini instructions
- `.cursorrules` _(spec)_ — Cursor rules
- `.windsurfrules` _(spec)_ — Windsurf rules
- `.github/copilot-instructions.md` _(spec)_ — GitHub Copilot instructions (.github)
- `AGENT.md` _(spec)_ — Multi-agent instructions

## Working files (current focus)
- `docs/register-maps.md` _(auto:recent-edit)_
- `docs/graphics.md` _(auto:recent-edit)_
- `docs/sources.md` _(auto:recent-edit)_
- `docs/io.md` _(auto:recent-edit)_
- `docs/cpu-memory.md` _(auto:recent-edit)_
- `docs/phase1-checklist.md` _(auto:recent-edit)_

## How to use this handoff
1. Read every file under "Spec / context files" before acting.
2. Continue the work described in the most recent thought.
3. Honor skill statuses: `ENABLED` use freely, `ASK` require explicit user confirmation each time, `DISABLED` must not be used.
4. When you reach a non-trivial decision, append a thought to `.aicb/state.json` (modelId + text + sourceReference if relevant).
<!-- AICB:END -->
