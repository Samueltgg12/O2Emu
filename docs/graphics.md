# O2 (IP32) Graphics — CRM Chipset

The O2 graphics subsystem is the **CRM chipset** (Cobalt/RM? — the graphics
chipset). It consists of four ASICs:

| ASIC | Role |
|------|------|
| **Microprocessor** | Display list / vertex processing + MRE control |
| **ICE** | Imaging & Compression Engine — pixel packaging/unpacking |
| **MRE** | Memory & Rendering Engine — rasterization + texture mapping |
| **Display Engine** | Analog video generation |

## Unified Memory Architecture

The O2 has **no separate VRAM**. The framebuffer and textures live in main
memory, accessed over the UMA bus. This is the key architectural difference
from the earlier SGI Indy (which had a separate graphics board).

## Microprocessor

- Display list / vertex processing
- Controls the MRE

## ICE (Imaging & Compression Engine)

- Pixel packaging / unpacking
- Handles image data transfer between the framebuffer and main memory

## MRE (Memory & Rendering Engine)

- Rasterization
- Texture mapping
- Contains the memory controller

## Display Engine

- Analog video generation
- Drives the video output

## Linux driver

- `drivers/video/crmfb.c` — framebuffer driver
- `drivers/video/crmfbreg.h` — register definitions

## NetBSD driver

- `sys/arch/sgimips/dev/crmfb.c` + `crmfbreg.h`

## IRIX source (leaked) — real GBE register definitions

The leaked IRIX source (`stand/arcs/ide/IP32/graphics/crmGfxState.c`) contains
the **real O2 framebuffer (GBE) register definitions** used by the PROM's
graphics init:

- `GBE_FRM_DEPTH_8` / `GBE_FRM_DEPTH_16` / `GBE_FRM_DEPTH_32` — framebuffer depth
- `GBE_FRM_DEPTH_SHIFT` / `GBE_FRM_DEPTH_MASK` — depth field in `outputFrm0`
- `GBE_FRM_HEIGHT_PIX_SHIFT` / `GBE_FRM_HEIGHT_PIX_MASK` — height in `outputFrm1`
- `IDE_FB_TILE_BASE` — framebuffer tile base address
- `IDE_FB_DLIST1_BASE` — display list base address

### Tile-based framebuffer layout

The O2 framebuffer is **tile-based** (not a linear scanline buffer):

- Normal tiles: 512 px wide (divided by depth bytes), 128 px tall
- Overlay tiles: 512 px wide, 128 px tall
- Overlay descriptor list is built in `crmGfxState.c`:
  `crmOverlayPtr[i] = (IDE_FB_TILE_BASE + (0x00010000 * (i + maxNormalNumTiles + 1))) >> 16`
- Supported depths: 8, 16, 32 bits
- 1600SW flat panel mode: 1600x1024 @ 50 Hz, 32-bit depth

### CRIME graphics init sequence (`stand/arcs/lib/libsk/graphics/CRIME/crm_init.c`)

`initGraphics()` performs, in order:
1. `crmInitGraphicsBase()`
2. `crmGetRev()` — reads CRIME revision (`0x000000a1`)
3. `initDisplay()`
4. `initCrime()`
5. `initGammaMap()`
6. `turnOnGbe()`
7. `initFramebuffer()`
8. `initTiming()`
9. `initCursor()`

## TODO / Open questions

- [ ] Full ICE register map
- [ ] Full MRE register map
- [ ] Full Display Engine register map
- [ ] crmfb framebuffer register details
- [ ] Video modes / timing details