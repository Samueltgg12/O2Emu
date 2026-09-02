# O2 (IP32) Research Sources

## Linux kernel

The Linux kernel has the most complete open-source register-level documentation
for the O2.

- `arch/mips/sgi-ip32/` — platform code
  - `ip32-berr.c` — bus error handling
  - `ip32-irq.c` — interrupt setup
  - `ip32-platform.c` — platform setup
  - `ip32-setup.c` — setup
  - `ip32-reset.c` — reset
  - `crime.c` — CRIME driver
  - `ip32-memory.c` — memory setup
  - `ip32-dma.c` — DMA setup
- `arch/mips/include/asm/ip32/` — headers
  - `crime.h` — CRIME register definitions
  - `mace.h` — MACE register definitions
  - `ip32_ints.h` — interrupt map
- `drivers/video/fbdev/gbefb.c` — GBE framebuffer driver
- `include/video/gbe.h` — GBE register definitions (`struct sgi_gbe`)
- `drivers/scsi/` — SCSI (aic7xxx)

## NetBSD

- `sys/arch/sgimips/` — platform code
  - `conf/GENERIC32_IP3x` — device tree
  - `dev/mace.c` — MACE driver
  - `dev/crime.c` — CRIME driver
  - `dev/crmfb.c` + `crmfbreg.h` — framebuffer
  - `dev/imc.c` + `imcreg.h` — memory controller
  - `dev/hpcreg.h` — HPC register definitions
  - `include/machtype.h` — MACH_SGI_IP32 = 32 "O2 Moosehead"

## Web sources

- `sgidepot.co.uk/o2cpumod.html` — CPU upgrade (600 MHz RM7xxx retrofit)
- `forums.irixnet.org thread-4794` — 128 MiB/stick DIMM reverse-engineering

## PROM firmware

- **`samples/ip32prom.rev4.18.bin`** — PROM image (MD5 `c9725e036052cf1f3e6258eb9bc687fa`)
- **`samples/ip32prom.rev4.3.bin`** — older PROM image (MD5 `6b86b20727a598ed15d93f27c9a3f2e8`)
- **`samples/decompiled-prom/`** — decompiled PROM output (MIPS assembly)
  - `rev4.18/` and `rev4.3/` — per-revision output
  - `definitions.h` — full IP32 address map + CRIME/MACE/UART/RTC registers
  - `post1.S`, `sloader.S`, `env.S`, `firmware.S`, `version.S`, `trailing.S`
- `github.com/mattst88/ip32prom-decompiler` — Rust tool that decompiles the IP32
  PROM (parses 5-section SHDR layout, firmware VMA `0x81000000`, embedded ELF
  header, two's complement checksum). Motivation: 900 MHz RM7900 CPU upgrade.
- Author's blog post — reverse-engineering write-up of the PROM structure.
- `forums.sgi.sh` thread 1508 — O2 PROM / firmware discussion.
- Hackaday / Adafruit / LavX articles — O2 CPU upgrade and PROM work.

## Datasheets & hardware references

Collected under [`docs/datasheets/`](datasheets/):

- **NEC VR5000/VR10000 Instruction User's Manual** — `datasheets/CPU/r5000 and
  r10000/DSA-446416.pdf` (+ `.md`). NEC doc U12754EJ1V0UMJ1 (Aug 2000). Full
  MIPS III/IV instruction set for the VR5000 (μPD30500) and VR10000 (μPD30700).
- **NEC VRSeries Programming Guide** — `datasheets/CPU/r120000/DSA-446414.pdf`
  (+ `.md`). NEC doc U10710EJ5V0AN00 (Nov 2001). Covers VR4100/VR4300/VR5000/
  VR5432/VR5500/VR10000/VR12000: pipeline, caches, TLB, CP0 registers, cache
  init/writeback/fill programming.
- **MIPS R5000/R10000/R12000** — NEC VRSeries manuals, collected under
  `docs/datasheets/CPU/`
- **Adaptec AIC-7880** — UltraWide SCSI controller datasheet (to collect)
- **SGI O2** — SGI hardware documentation (owner's manual, field service; to
  collect or verify)
- **SDRAM DIMM** — JEDEC 239-pin SDRAM DIMM spec (O2-specific reference still
  to collect or verify)

> SPD status: no local O2-specific SPD address, EEPROM layout, or PROM DIMM
> probe sequence has been verified. The PROM I2C code found so far is used for
> display/flat-panel probing, so this remains an open research item.

> The leaked IRIX source remains the primary register-level reference; the
> NEC VRSeries datasheets are the authoritative CPU architecture reference.

## Leaked IRIX source

- **`github.com/calmsacibis995/irix-657m-src`** — IRIX 6.5.7m source code,
  released February 10, 2000. Top-level dirs: `eoe/`, `irix/`, `stand/`, plus
  `007-3897-006.pdf` (release notes) and `README.md`.
  - `irix/kern/` — kernel (`io/io6.c` I/O subsystem, `io/rad1/rad1drv.c` RAID,
    `bsd/netinet/` IPv6, `fs/dfs/` DFS/RPC, `sys/ISPcode1.27.h` SCSI microcode)
  - `irix/cmd/` — commands (`xfs/repair/`, `xlv/`, `icrash/`, `snmp/`, `netman/`)
  - `irix/lib/` — libraries (`librestart/` checkpoint/restart, `libirixpmda/` PCP)
  - `eoe/cmd/` — EOE commands (`coffcheck/`, `pcp/`, `top/`, `react/`)
  - `stand/x86/ffsc/` — x86 FFSC (VxWorks-based) source
  - **`stand/arcs/IP32prom/`** — the actual IP32 PROM source code
  - **`stand/arcs/ide/IP32/`** — IP32 IDE (diagnostics): `graphics/crmGfxState.c`
    (real GBE framebuffer registers), `mace/siodiag2.c` (UART), `mem/khlow_diag.s`
  - **`stand/arcs/lib/libsk/graphics/CRIME/crm_init.c`** — CRIME graphics init
  - Build system: `smake` with `#!smake` Makefiles, `OBJECT_STYLE=n32`,
    `#ident "$Revision: ... $"` SCCS identifiers.
  - 6.5.4 introduced the 270 MHz O2/Octane processor; 6.5.3 introduced R12000.

- **`github.com/jacklin9/IRIX-6.5.17-Src`** — IRIX 6.5.17 source code (later
  release). Contains the authoritative register headers for O2:
  - `irix/kern/sys/mace.h` — **authoritative MACE register map** (PCI, Ethernet,
    Audio, I2C, PS/2, ISA, UST/MSC, interrupt assignments)
  - `irix/kern/sys/crime.h` — **authoritative CRIME CPU interface register map**
    (control, interrupts, timers, memory controller, error registers)
  - `irix/kern/sys/IP32.h` — IP32 platform definitions, `VICE_CPU_INTR = 31`
  - `stand/arcs/IP32prom/include/sys/crimereg.h` — **authoritative CRIME RE
    register map** (Interface Buffer, TLB, Pixel Pipe, MTE, Status)
  - `stand/arcs/IP32prom/include/sys/crimedef.h` — **authoritative CRIME RE
    bitfield definitions** (buffer modes, draw modes, primitive types, etc.)
  - `stand/arcs/IP32prom/include/sys/crimechip.h` — CRIME chip register layout
  - `stand/arcs/IP32prom/include/sys/crime_gbe.h` — **authoritative GBE
    register map** (control, video timing, overlay, framebuffer, DID, WID,
    colormap, gamma, cursor, video capture)
  - `stand/arcs/IP32prom/include/sys/crime_gfx.h` — GBE/CRM graphics ioctls
  - `stand/arcs/IP32prom/include/sys/gbedefs.h` — GBE mode register bitfields
  - `stand/arcs/IP32prom/include/crm_timing.h` — video timing tables
  - `stand/arcs/IP32prom/include/crm_stand.h` — standalone CRIME definitions
  - `stand/arcs/ide/IP32/graphics/` — diagnostics: `crmGBECommands.c`,
    `crmGfxState.c`, `crmRECommands.c`, `crmRegTest.c`, `crmVisualTest.c`
  - **Key finding**: The ICE ASIC is called **VICE** (Video Image Compression
    Engine) in IRIX. Interrupt 31. Error address at `CRM_VICE_ERROR_ADDR`.
    No authoritative VICE register header located in this source tree.

- **`github.com/jacklin9/IRIX-6.5.17-Src`** — IRIX 6.5.17 source tree, the newer
  leak and a better reference for later IP32-era code and system definitions.
  This is a useful follow-up to the 6.5.7m leak when comparing platform drivers,
  firmware, and system architecture changes across the 6.5.x line.
  - Contains the later IRIX sources for the same SGI IP32 and related systems.
  - Useful for confirming whether IP32 PROM, crm, MACE, and graphics code changed
    between 6.5.7m and 6.5.17.
  - Should be treated as the same category of research artifact as the 6.5.7m leak.

## TODO / Open questions

- [ ] Full ICE register map (no authoritative header located yet)
- [x] CRIME/MRE/Display/GBE register maps (`definitions.h`, Linux `gbe.h`)
- [ ] Cross-check the GBE map against leaked IRIX `crmGfxState.c`
- [x] PROM-level MACE Ethernet/audio register observations (`firmware.S`)
- [ ] Complete mavb audio register details and codec semantics
- [ ] DIMM SPD address, EEPROM layout, and probe behavior
- [ ] Search IRIX source for ASIC drivers lacking open-source coverage
      (ICE, Ethernet packet engine, mavb, and codec) in `irix/kern/io/`