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
- `drivers/video/crmfb.c` + `crmfbreg.h` — framebuffer driver
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
- `github.com/mattst88/ip32prom-decompiler` — Rust tool that decompiles the IP32
  PROM (parses 5-section SHDR layout, firmware VMA `0x81000000`, embedded ELF
  header, two's complement checksum). Motivation: 900 MHz RM7900 CPU upgrade.
- Author's blog post — reverse-engineering write-up of the PROM structure.
- `forums.sgi.sh` thread 1508 — O2 PROM / firmware discussion.
- Hackaday / Adafruit / LavX articles — O2 CPU upgrade and PROM work.

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

- **`github.com/jacklin9/IRIX-6.5.17-Src`** — IRIX 6.5.17 source tree, the newer
  leak and a better reference for later IP32-era code and system definitions.
  This is a useful follow-up to the 6.5.7m leak when comparing platform drivers,
  firmware, and system architecture changes across the 6.5.x line.
  - Contains the later IRIX sources for the same SGI IP32 and related systems.
  - Useful for confirming whether IP32 PROM, crm, MACE, and graphics code changed
    between 6.5.7m and 6.5.17.
  - Should be treated as the same category of research artifact as the 6.5.7m leak.

## TODO / Open questions

- [ ] Full CRIME/MACE/ICE/MRE/Display register maps
- [ ] crmfb framebuffer register details
- [ ] mavb audio register details
- [ ] Search IRIX source for ASIC drivers lacking open-source coverage
      (ICE, MRE, Display Engine, crmfb, mavb) in `irix/kern/io/`