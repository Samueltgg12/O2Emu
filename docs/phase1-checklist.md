# Phase 1 Research Checklist

Phase 1 is complete when every hardware subsystem has a sourced register map
and the physical memory map is documented. No emulator implementation belongs
in this phase.

## Hardware documentation

- [x] System architecture and UMA overview
- [x] CPU families, CP0, caches, TLB, and CPU identification
- [x] CRIME memory controller and interrupt registers
- [x] PROM-visible MRE/RE/DE/MTE registers
- [x] GBE / Display Engine register map
- [ ] ICE register map (ICE = VICE in IRIX; only interrupt/error registers
      known — no authoritative VICE header located)
- [x] PROM-level MACE Ethernet register observations
- [x] PROM-level MACE audio ring and codec register observations
- [x] AD1843 codec register map (IRIX `ad1843.h`)
- [x] AIC-7880 SCSI references (IRIX `adp78.h`/`adp78.c`, Linux `aic7xxx/`)
- [x] SDRAM/DIMM bank registers and PROM probing code (CRIME bank control)
- [ ] Complete MACE Ethernet packet and descriptor semantics
- [ ] Complete MACE audio codec and `mavb` semantics
- [ ] DIMM SPD address, EEPROM layout, and probing behavior
- [ ] Per-mode GBE timing tables and monitor behavior
- [ ] PCI configuration-space details
- [x] UART, RTC, PS/2, I2C, and physical address maps
- [x] PROM layout, reset path, POST, and firmware behavior

## Source and consistency review

- [ ] Cross-check every register table against PROM, Linux, NetBSD, or IRIX
- [ ] Record all newly discovered repositories, datasheets, and documents
- [ ] Remove stale or contradictory source paths and claims
- [ ] Complete documentation indexes and cross-links
- [ ] Review the Phase 1 exit criteria
- [ ] Freeze the research baseline before Phase 2 scaffolding

## Phase transition

- [ ] Update `ROADMAP.md` to mark Phase 1 complete
- [ ] Define Phase 2 emulator scope and first boot milestone
- [ ] Create the C++/CMake project only after the research baseline is frozen