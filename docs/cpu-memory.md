# O2 (IP32) CPU & Memory

## CPU options

| CPU | Speed | Notes |
|-----|-------|-------|
| MIPS R5000 | 180–350 MHz | 1 MB L2 cache |
| MIPS RM7000 | low-end | |
| MIPS R10000 | 150–400 MHz | |
| MIPS R12000 | high-end | |

- 200 MHz R5000 + 1 MB L2 is faster than 180 MHz + 512 KB
- Hobbyist retrofit: 600 MHz RM7xxx (sgidepot.co.uk/o2cpumod.html)

## Memory

- **8 DIMM slots**, proprietary 239-pin SDRAM DIMMs
- Up to **1 GB** total
- Up to **128 MiB/stick** (reverse-engineered; forums.irixnet.org thread-4794)
- **ECC** supported

## Memory controller (CRIME)

CRIME is the memory controller / system controller ASIC.

- 133 MHz 144-bit bus (128-bit data + ECC)
- Buffered to a 66 MHz 256-bit memory system
- Provides the top-level interrupt controller
- See [register-maps.md](register-maps.md) for CRIME registers

## Memory & Rendering Engine (MRE)

- Contains the memory controller
- Rasterization + texture mapping
- UMA: textures and framebuffer come from main memory

## Linux memory setup

- `arch/mips/sgi-ip32/ip32-memory.c` — memory setup
- `arch/mips/sgi-ip32/ip32-dma.c` — DMA setup
- `arch/mips/sgi-ip32/crime.c` — CRIME driver

## NetBSD memory setup

- `sys/arch/sgimips/dev/imc.c` + `imcreg.h` — IMC (memory controller) driver
- `sys/arch/sgimips/dev/crime.c` — CRIME driver

## TODO / Open questions

- [ ] Full CRIME memory controller register map (ECC, refresh, timing)
- [ ] MRE register map
- [ ] DIMM SPD / configuration details
- [ ] Exact memory map (physical address layout)