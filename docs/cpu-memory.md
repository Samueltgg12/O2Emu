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

### CRIME base addresses (decompiled PROM `definitions.h`)

- `PHYS_BASE_CRIME = 0x14000000` (CRIME CPU interface)
- `PHYS_BASE_RENDER = 0x15000000` (render engine interface)

> Note: an older IRIX `crm_stand.h` lists the render base as `0x14100000`;
> the decompiled PROM (authoritative) uses `0x15000000`.

### CRIME memory controller registers (IRIX `stand/arcs/IP32prom/include/sys/crime.h`)

Offsets are relative to `CRM_BASEADDR`:

| Register | Offset | Notes |
|----------|--------|-------|
| `CRM_MEM_CONTROL` | `0x200` | 2-bit; `ECC_ENA` (0x10), `USE_ECC_REPL` (0x20) |
| `CRM_MEM_BANK_CTRL(x)` | `0x208 + x*8` | 8 banks; `SDRAM_SIZE` (0x100), `ADDR` (0x1f) |
| `CRM_MEM_REFRESH_CNTR` | `0x248` | 11-bit refresh counter |
| `CRM_MEM_ERROR_STAT` | `0x250` | 28-bit error status |
| `CRM_MEM_ERROR_ADDR` | `0x258` | error address (bits 29:0) |
| `CRM_MEM_ERROR_ECC_SYN` | `0x260` | ECC syndrome |
| `CRM_MEM_ERROR_ECC_CHK` | `0x268` | generated ECC check bits |
| `CRM_MEM_ERROR_ECC_REPL` | `0x270` | ECC replacement bits |

- Bank address decode: `CRM_MEM_BANK_CTRL_BANK_TO_ADDR(x) = ((x & 0x1f) << 25)`
- Memory error address correction (IRIX `irix/kern/ml/error.c`): CRIME reports
  only bits 29:0. Memory below 256 MB is accessed at the 0-based alias; memory
  at/above 256 MB is accessed above 1 GB, so `if (physaddr >= 0x10000000)
  physaddr += 0x40000000`.

### Memory banks

- Up to 8 banks (`CRM_MAXBANKS = 8`)
- Each bank is either **32 MB** or **128 MB** (64 Mbit SDRAM)
- POST sizes banks by probing (IRIX `stand/arcs/IP32prom/post/post1mem.c`)

## Physical memory map

| Range | Purpose |
|-------|---------|
| `0x00000000`–`0x0fffffff` | Main memory (up to 256 MB, 0-based alias) |
| `0x10000000`–`0x13ffffff` | Memory above 256 MB (aliased above 1 GB) |
| `0x14000000` | CRIME CPU interface |
| `0x15000000` | Render engine interface |
| `0x1f000000` | MACE (I/O engine) |
| `0x1fc00000` | PROM (KSEG1 view of reset vector `0xBFC00000`) |

Source: decompiled PROM `definitions.h` (`PHYS_BASE_CRIME`, `PHYS_BASE_RENDER`,
`PHYS_BASE_MACE`, `PHYS_SYSTEM_ROM`).

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

- [ ] MRE register map
- [ ] DIMM SPD / configuration details
- [ ] Full CRIME timing/refresh programming sequence