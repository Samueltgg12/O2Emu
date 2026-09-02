# O2 (IP32) Register Maps — from Linux/NetBSD drivers

Register-level details mined from open-source drivers. These are the ground
truth for emulator implementation.

## Memory Map (Linux `arch/mips/include/asm/ip32/mace.h`)

- `MACE_BASE = 0x1f000000` (physical)

## CRIME ASIC (Linux `arch/mips/include/asm/ip32/crime.h`)

CRIME is the memory controller / system controller ASIC.

### CRIME base addresses (IRIX `stand/arcs/IP32prom/include/crm_stand.h`)

- `CRM_CPU_INTERFACE_BASE_ADDRESS = 0x14000000`
- `CRM_RE_BASE_ADDRESS = 0x14100000`

### CRIME register map (IRIX `stand/arcs/IP32prom/debugcard/include/crimereg.h`)

Offsets are relative to the CRIME base:

| Register | Offset |
|----------|--------|
| `id` | `0x00` |
| `cntl` | `0x08` |
| `intrpt_status` | `0x10` |
| `intrpt_enable` | `0x18` |
| `soft_intrpt` | `0x20` |
| `hard_intrpt` | `0x28` |
| `watch_dog` | `0x30` |
| `crime_timer` | `0x38` |
| `cpu_error_addr` | `0x40` |
| `cpu_error_status` | `0x48` |
| `vice_error_addr` | `0x58` |
| `bank0_cntl` … `bank7_cntl` | `0x208` … `0x238` |
| `refresh_cntr` | `0x248` |
| `mem_error_status` | `0x250` |
| `mem_error_addr` | `0x258` |
| `ecc_syndrome` | `0x260` |
| `ecc_check_bits` | `0x268` |
| `ecc_repl` | `0x270` |

### CRIME interrupt bits (IRIX `stand/arcs/IP32prom/include/sys/crime.h`)

```text
CRM_INT_RE1      0x00800000
CRM_INT_RE0      0x00400000
CRM_INT_MEMERR   0x00200000
CRM_INT_CRMERR   0x00100000
CRM_INT_GBE3     0x00080000
CRM_INT_GBE2     0x00040000
CRM_INT_GBE1     0x00020000
CRM_INT_GBE0     0x00010000
CRM_INT_MACE(i)  (1 << i)
```

### CRIME GBE (framebuffer) registers (IRIX `stand/arcs/IP32prom/include/sys/crime_gbe.h`)

Offsets are relative to the GBE register block:

| Register | Offset |
|----------|--------|
| `vt_vcmap` | `0x10040` |
| `did_start_xy` | `0x10044` |
| `crs_start_xy` | `0x10048` |
| `vc_start_xy` | `0x1004c` |
| `ovr_width_tile` | `0x20000` |
| `ovr_control` | `0x20004` |
| `frm_size_tile` | `0x30000` |
| `frm_size_pixel` | `0x30004` |

### GBE (Display Engine) register map (Linux `include/video/gbe.h`)

The GBE is the Display Engine ASIC. `GBE_BASE = 0x16000000` (SGI O2), register
block is 1 MiB (`0x100000`). Offsets below are relative to `GBE_BASE`. The
register block is exposed as `struct sgi_gbe` in `include/video/gbe.h`; the
driver is `drivers/video/fbdev/gbefb.c`.

**Control / clock / ID block** (offset `0x00000`):

| Register | Offset | Fields |
|----------|--------|--------|
| `ctrlstat` | `0x00000` | `CHIPID` 3:0, `SENSE_N` 4, `PCLKSEL` 29:28 |
| `dotclock` | `0x00004` | `M` 7:0, `N` 13:8, `P` 15:14, `RUN` 20 |
| `i2c` | `0x00008` | I2C bus |
| `sysclk` | `0x0000c` | system clock |
| `i2cfp` | `0x00010` | I2C flat-panel |
| `id` | `0x00014` | chip ID |
| `config` | `0x00018` | configuration |
| `bist` | `0x0001c` | built-in self test |

**Video timing block** (offset `0x10000`):

| Register | Offset | Fields |
|----------|--------|--------|
| `vt_xy` | `0x10000` | `X` 11:0, `Y` 23:12, `FREEZE` 31 |
| `vt_xymax` | `0x10004` | `MAXX` 11:0, `MAXY` 23:12 |
| `vt_vsync` | `0x10008` | vertical sync |
| `vt_hsync` | `0x1000c` | horizontal sync |
| `vt_vblank` | `0x10010` | vertical blank |
| `vt_hblank` | `0x10014` | horizontal blank |
| `vt_flags` | `0x10018` | `VDRV_INVERT` 0, `VDRV_LOW` 1, `HDRV_INVERT` 2, `HDRV_LOW` 3, `SYNC_HIGH` 4, `SYNC_LOW` 5, `F2RF_HIGH` 6 |
| `vt_f2rf_lock` | `0x1001c` | frame-to-raster lock |
| `vt_intr01` | `0x10020` | interrupt 0/1 |
| `vt_intr23` | `0x10024` | interrupt 2/3 |
| `fp_hdrv` | `0x10028` | flat-panel hdrv |
| `fp_vdrv` | `0x1002c` | flat-panel vdrv |
| `fp_de` | `0x10030` | flat-panel data enable |
| `vt_hpixen` | `0x10034` | horizontal pixel enable |
| `vt_vpixen` | `0x10038` | vertical pixel enable |
| `vt_hcmap` | `0x1003c` | horizontal colormap |
| `vt_vcmap` | `0x10040` | vertical colormap |
| `did_start_xy` | `0x10044` | `DID_STARTX` 11:0, `DID_STARTY` 23:12 |
| `crs_start_xy` | `0x10048` | `CRS_STARTX` 11:0, `CRS_STARTY` 23:12 |
| `vc_start_xy` | `0x1004c` | `VC_STARTX` 11:0, `VC_STARTY` 23:12 |

**Overlay plane** (offset `0x20000`):

| Register | Offset | Fields |
|----------|--------|--------|
| `ovr_width_tile` | `0x20000` | `OVR_FIFO_RESET` 13 |
| `ovr_inhwctrl` | `0x20004` | `OVR_DMA_ENABLE` 0 |
| `ovr_control` | `0x20008` | `OVR_DMA_ENABLE` 0 |

**Normal framebuffer plane** (offset `0x30000`):

| Register | Offset | Fields |
|----------|--------|--------|
| `frm_size_tile` | `0x30000` | `FRM_RHS` 4:0, `FRM_WIDTH_TILE` 12:5, `FRM_DEPTH` 14:13, `FRM_FIFO_RESET` 15 |
| `frm_size_pixel` | `0x30004` | `FB_HEIGHT_PIX` 31:16 |
| `frm_inhwctrl` | `0x30008` | `FRM_DMA_ENABLE` 0 |
| `frm_control` | `0x3000c` | `FRM_DMA_ENABLE` 0, `FRM_LINEAR` 1, `FRM_TILE_PTR` 31:9 |

**DID control** (offset `0x40000`):

| Register | Offset | Fields |
|----------|--------|--------|
| `did_inhwctrl` | `0x40000` | `DID_DMA_ENABLE` 0 |
| `did_control` | `0x40004` | `DID_DMA_ENABLE` 0 |

**WID table** (offset `0x50000`): `mode_regs[32]`, fields `BUF` 1:0, `TYP` 4:2,
`CM` 9:5, `GAMMA` 10, `AUX` 12:11.

**Color / gamma map** (offset `0x60000`):

| Register | Offset |
|----------|--------|
| `cmap[6144]` | `0x60000` |
| `cm_fifo` | `0x78000` |
| `gmap[256]` | `0x80000` |
| `gmap10[1024]` | `0x90000` |

**Cursor** (offset `0xa0000`):

| Register | Offset |
|----------|--------|
| `crs_pos` | `0xa0000` |
| `crs_ctl` | `0xa0004` |
| `crs_cmap[3]` | `0xa0008` |
| `crs_glyph[64]` | `0xa0014` |

**Video capture** (offset `0xb0000`): `vc_0` … `vc_8`.

**Color-mode constants** (`GBE_CMODE_*`): `I8` 0, `I12` 1, `RG3B2` 2, `RGB4` 3,
`ARGB5` 4, `RGB8` 5, `RGBA5` 6, `RGB10` 7.

**Driver init sequence** (`gbefb.c`): read `gbe_revision = ctrlstat & 15`; build
tile list; init WID table; `vt_intr01`/`vt_intr23 = 0xffffffff`;
`did_control`/`ovr_width_tile`/`crs_ctl = 0`; init gamma map; init color map +
`gbe_loadcmap()`; program `vt_xymax`/`vsync`/`hsync`/`vblank`/`hblank`;
`vc_start_xy` with `VC_STARTX = hblank_end - 4`; `vt_hpixen`/`vt_vpixen`;
`did_start_xy` with `DID_STARTX = hblank_end - 20`; `crs_start_xy` with
`CRS_STARTY = temp + 1`, `CRS_STARTX = hblank_end - GBE_CRS_MAGIC`;
`fp_de`/`hdrv`/`vdrv`; `frm_size_pixel` with `FB_HEIGHT_PIX`;
`frm_size_tile` with `FRM_WIDTH_TILE`/`FRM_RHS`/`FRM_DEPTH`; then
`ctrlstat`/`dotclock`/`sysclk`/`i2c`/`i2cfp`/`id`/`config`/`bist`.

**Timing info** (`struct gbe_timing_info`): `flags`, `width`, `height`,
`fields_sec`, `cfreq`, `htotal`, `hblank_start`/`hblank_end`,
`hsync_start`/`hsync_end`, `vtotal`, `vblank_start`/`vblank_end`,
`vsync_start`/`vsync_end`, `pll_m`/`pll_n`/`pll_p`. Flags: `GBE_VOF_UNKNOWNMON`
1, `STEREO` 2, `DO_GENSYNC` 4, `SYNC_ON_GREEN` 8, `FLATPANEL` 0x1000,
`MAGICKEY` 0x2000.

### CRIME Control register bits

```text
CRIME_CONTROL_TRITON_SYSADC  0x2000
CRIME_CONTROL_CRIME_SYSADC   0x1000
CRIME_CONTROL_HARD_RESET     0x0800
CRIME_CONTROL_SOFT_RESET     0x0400
CRIME_CONTROL_DOG_ENA        0x0200
CRIME_CONTROL_ENDIANESS      0x0100
CRIME_CONTROL_ENDIAN_BIG     0x0100
CRIME_CONTROL_ENDIAN_LITTLE  0x0000
CRIME_CONTROL_CQUEUE_HWM     0x000f   (command queue high-water mark)
CRIME_CONTROL_CQUEUE_SHFT    0
CRIME_CONTROL_WBUF_HWM       0x00f0   (write buffer high-water mark)
CRIME_CONTROL_WBUF_SHFT      8
```

### CRIME interrupt registers
```
istat      (interrupt status)
imask      (interrupt mask)
soft_int   (software interrupt)
hard_int   (hardware interrupt)
```

### CRIME interrupt bits (hard_int)
```
MACE_VID_IN1_INT        BIT(0)
MACE_VID_IN2_INT        BIT(1)
MACE_VID_OUT_INT        BIT(2)
MACE_ETHERNET_INT       BIT(3)
MACE_SUPERIO_INT        BIT(4)
MACE_MISC_INT           BIT(5)
MACE_AUDIO_INT          BIT(6)
MACE_PCI_BRIDGE_INT     BIT(7)
MACEPCI_SCSI0_INT       BIT(8)
MACEPCI_SCSI1_INT       BIT(9)
MACEPCI_SLOT0_INT       BIT(10)
MACEPCI_SLOT1_INT       BIT(11)
MACEPCI_SLOT2_INT       BIT(12)
MACEPCI_SHARED0_INT     BIT(13)
MACEPCI_SHARED1_INT     BIT(14)
MACEPCI_SHARED2_INT     BIT(15)
CRIME_GBE0_INT          BIT(16)
CRIME_GBE1_INT          BIT(17)
CRIME_GBE2_INT          BIT(18)
CRIME_GBE3_INT          BIT(19)
CRIME_CPUERR_INT        BIT(20)
CRIME_MEMERR_INT        BIT(21)
```

### CRIME interrupt classes (from `ip32-irq.c`)
- **Level-triggered:** `CRIME_CPUERR_IRQ`, `CRIME_MEMERR_IRQ`
- **Edge-triggered:** `CRIME_GBE0..GBE3`, `CRIME_RE_EMPTY_E..RE_IDLE_E`,
  `CRIME_SOFT0..SOFT2`, `CRIME_VICE_IRQ`

## MACE ASIC (Linux `arch/mips/include/asm/ip32/mace.h`)

MACE = Multimedia, Audio and Communications Engine.

### MACE PCI interface
```
struct mace_pci {
    error_addr;
    error;
    MACEPCI_ERROR_MASTER_ABORT      BIT(31)
    MACEPCI_ERROR_TARGET_ABORT      BIT(30)
    MACEPCI_ERROR_DATA_PARITY_ERR   BIT(29)
    MACEPCI_ERROR_RETRY_ERR         BIT(28)
    MACEPCI_ERROR_ILLEGAL_CMD       BIT(27)
    MACEPCI_ERROR_SYSTEM_ERR        BIT(26)
}
```

## Interrupt map (Linux `arch/mips/include/asm/ip32/ip32_ints.h`)

### CRIME interrupts
```
CRIME_VICE_IRQ
```

### MACEISA interrupts
```
MACEISA_AUDIO_SW_IRQ
MACEISA_AUDIO_SC_IRQ
MACEISA_AUDIO1_DMAT_IRQ
MACEISA_AUDIO1_OF_IRQ
MACEISA_AUDIO2_DMAT_IRQ
MACEISA_AUDIO2_MERR_IRQ
MACEISA_AUDIO3_DMAT_IRQ
MACEISA_AUDIO3_MERR_IRQ
MACEISA_RTC_IRQ
MACEISA_KEYB_IRQ
MACEISA_MOUSE_IRQ
MACEISA_TIMER0_IRQ
MACEISA_TIMER1_IRQ
MACEISA_TIMER2_IRQ
MACEISA_PARALLEL_IRQ
MACEISA_PAR_CTXA_IRQ
MACEISA_PAR_CTXB_IRQ
MACEISA_PAR_MERR_IRQ
MACEISA_SERIAL1_IRQ
MACEISA_SERIAL1_TDMAT_IRQ
MACEISA_SERIAL1_TDMAPR_IRQ
MACEISA_SERIAL2_RDMAOR_IRQ
...
IP32_IRQ_MAX = MACEISA_SERIAL2_RDMAOR_IRQ
```

### MACEISA interrupt groups (from `ip32-irq.c`)
```
MACEISA_AUDIO_INT = AUDIO_SW | AUDIO_SC | AUDIO1_DMAT | AUDIO1_OF |
                    AUDIO2_DMAT | AUDIO2_MERR | AUDIO3_DMAT | AUDIO3_MERR
MACEISA_MISC_INT  = RTC | KEYB | KEYB_POLL | MOUSE | MOUSE_POLL |
                    TIMER0 | TIMER1 | TIMER2
```

## PCI device map (Linux `arch/mips/pci/fixup-ip32.c`)

O2 has up to 5 PCI devices connected into the MACE bridge:
```
0  aic7xxx 0   (SCSI)
1  aic7xxx 1   (SCSI)
2  expansion slot
3  N/C
4  N/C
```

IRQ routing:
```
SCSI0 = MACEPCI_SCSI0_IRQ
SCSI1 = MACEPCI_SCSI1_IRQ
INTA0 = MACEPCI_SLOT0_IRQ
INTA1 = MACEPCI_SLOT1_IRQ
INTA2 = MACEPCI_SLOT2_IRQ
INTB  = MACEPCI_SHARED0_IRQ
INTC  = MACEPCI_SHARED1_IRQ
INTD  = MACEPCI_SHARED2_IRQ
```

## Linux IP32 kernel files (`arch/mips/sgi-ip32/`)

From `Makefile`:
```
obj-y += ip32-berr.o ip32-irq.o ip32-platform.o ip32-setup.o ip32-reset.o \
         crime.o ip32-memory.o ip32-dma.o
```

- `ip32-berr.c` — bus error handling
- `ip32-irq.c` — interrupt controller setup
- `ip32-platform.c` — platform devices
- `ip32-setup.c` — machine setup
- `ip32-reset.c` — reset/power
- `crime.c` — CRIME driver
- `ip32-memory.c` — memory setup
- `ip32-dma.c` — DMA

`flush_crime_bus()` does a PIO read to ensure no pending PIO writes.

## NetBSD sgimips files

- `sys/arch/sgimips/mace/mace.c` — MACE driver
- `sys/arch/sgimips/dev/crime.c` — CRIME driver
- `sys/arch/sgimips/dev/crmfb.c` + `crmfbreg.h` — framebuffer
- `sys/arch/sgimips/dev/imc.c` + `imcreg.h` — IMC (memory controller)
- `sys/arch/sgimips/conf/GENERIC32_IP3x` — device tree
- `sys/arch/sgimips/hpc/hpcreg.h` — HPC3 registers (Indy/Indigo2, NOT O2)

## Physical address map (decompiled PROM `definitions.h`)

The decompiled PROM (`samples/decompiled-prom/rev4.18/definitions.h`) is the
authoritative source for the IP32 address map:

| Base | Device |
|------|--------|
| `0x14000000` | CRIME (`PHYS_BASE_CRIME`) |
| `0x15000000` | Render engine (`PHYS_BASE_RENDER`) |
| `0x1f000000` | MACE (`PHYS_BASE_MACE`) |
| `0x1fc00000` | System ROM / PROM (`PHYS_SYSTEM_ROM`) |

MACE sub-offsets (relative to `0x1f000000`):

| Offset | Device |
|--------|--------|
| `0x080000` | PCI |
| `0x280000` | Ethernet |
| `0x300000` | Peripheral (Audio `0x00000`, ISA `0x10000`, KBD/MS `0x20000`, I2C `0x30000`, UST `0x40000`) |
| `0x380000` | ISA external (UART1 `0x10000`, UART2 `0x18000`, RTC `0x20000`) |

## CRIME registers (decompiled PROM `definitions.h`)

Offsets relative to `0x14000000`:

| Register | Offset |
|----------|--------|
| `CRIME_ID_OFFSET` | `0x0000` |
| `CRIME_CONTROL_OFFSET` | `0x0008` |
| `CRIME_INTSTAT_OFFSET` | `0x0010` |
| `CRIME_INTMASK_OFFSET` | `0x0018` |
| `CRIME_SOFT_INT_OFFSET` | `0x0020` |
| `CRIME_HARD_INT_OFFSET` | `0x0028` |
| `CRIME_WATCHDOG_OFFSET` | `0x0030` |
| `CRIME_TIMER_OFFSET` | `0x0038` |
| `CRIME_CPU_ERROR_ADDR` | `0x0040` |
| `CRIME_CPU_ERROR_STAT` | `0x0048` |
| `CRIME_CPU_ERROR_ENA` | `0x0050` |
| `CRIME_MC_STATUS_CTRL` | `0x0200` |
| `CRIME_BANK_0_CTRL` … `CRIME_BANK_7_CTRL` | `0x0208` … `0x0240` |
| `CRIME_REFRESH_COUNTER` | `0x0248` |
| `CRIME_ERROR_STATUS` | `0x0250` |
| `CRIME_ERROR_ADDR` | `0x0258` |
| `CRIME_SYNDROME_BITS` | `0x0260` |
| `CRIME_GENERATED_CHECK_BITS` | `0x0268` |
| `CRIME_REPLACEMENT_CHECK_BITS` | `0x0270` |

CRIME control bits: `TRITON_SYSADC` `0x2000`, `CRIME_SYSADC` `0x1000`,
`HARD_RESET` `0x0800`, `SOFT_RESET` `0x0400`.

### CRIME rendering engine registers

Offsets relative to the render base (`0x15000000`):

| Register | Offset |
|----------|--------|
| `RENDER_INTERFACE_CTRL` | `0x0400` |
| `CRIME_RE_TLB_A/B/C` | `0x1000`/`0x1200`/`0x1400` |
| `CRIME_DE_MODE_SRC` | `0x2000` |
| `CRIME_DE_MODE_DST` | `0x2008` |
| `CRIME_DE_DRAWMODE` | `0x2018` |
| `CRIME_DE_SCRMASK0-4` | `0x2020`–`0x2040` |
| `CRIME_DE_SCISSOR` | `0x2048` |
| `CRIME_DE_WINOFFSET_SRC/DST` | `0x2050`/`0x2058` |
| `CRIME_DE_PRIMITIVE` | `0x2060` |
| `CRIME_DE_X_VERTEX_0/1` | `0x2070`/`0x2074` |
| `CRIME_DE_XFER_STEP_X/Y` | `0x20a8`/`0x20ac` |
| `CRIME_DE_STIPPLE_MODE/PAT` | `0x20c0`/`0x20c4` |
| `CRIME_DE_FG` | `0x20d0` |
| `CRIME_DE_ROP` | `0x21b0` |
| `CRIME_DE_PLANEMASK` | `0x21b8` |
| `CRIME_DE_NULL` | `0x21f0` |
| `CRIME_DE_FLUSH` | `0x21f8` |
| `MTE_MODE` | `0x3000` |
| `MTE_BYTE_MASK` | `0x3008` |
| `MTE_STIPPLE_MASK` | `0x3010` |
| `MTE_FG_VALUE` | `0x3018` |
| `MTE_SRC0/1` | `0x3020`/`0x3028` |
| `MTE_DST0/1` | `0x3030`/`0x3038` |
| `MTE_SRC_Y_STEP` | `0x3040` |
| `MTE_DST_Y_STEP` | `0x3048` |
| `MTE_NULL` | `0x3070` |
| `MTE_FLUSH` | `0x3078` |
| `CRIME_DE_STATUS` | `0x4000` |
| `CRIME_DE_START` | `0x0800` |

## MACE registers (decompiled PROM `definitions.h`)

### PCI host bridge

| Register | Offset |
|----------|--------|
| `MACE_PCI_ERROR_ADDR` | `0x00` |
| `MACE_PCI_ERROR_FLAGS` | `0x04` |
| `MACE_PCI_CONTROL` | `0x08` |
| `MACE_PCI_CONFIG_ADDR` | `0xcf8` |
| `MACE_PCI_CONFIG_DATA` | `0xcfc` |

### Ethernet

| Register | Offset |
|----------|--------|
| `MACE_ETH_MAC_CONTROL` | `0x00` |
| `MACE_ETH_INTR_STATUS` | `0x08` |
| `MACE_ETH_RX_MCL_WR_PTR` | `0x45` |
| `MACE_ETH_RX_MCL_RD_PTR` | `0x46` |
| `MACE_ETH_RX_MCL_DEPTH` | `0x47` |
| `MACE_ETH_MCL_RECEIVE_FIFO(x)` | `x + 0x100` |

PROM observations (`samples/decompiled-prom/rev4.18/firmware.S`): the MAC
control register is accessed through the low 32-bit lane at `0x04` on the
big-endian bus; the receive FIFO is accessed at `0x104`; and the write pointer,
read pointer, and depth are byte registers at `0x45`, `0x46`, and `0x47`.
The PROM diagnostic exercises 16 receive-FIFO positions but does not expose a
complete packet descriptor or transmit-register map.

### Audio

| Register | Offset |
|----------|--------|
| `MACE_AUDIO_STATUS` | `0x00` |
| `MACE_AUDIO_CODEC_STATUS` | `0x08` |
| `MACE_AUDIO_CODEC_INPUT_MASK` | `0x10` |
| `MACE_AUDIO_CODEC_INPUT` | `0x18` |
| `MACE_AUDIO_RING_CTRL_CHAN(x)` | `0x20*x` |
| `MACE_AUDIO_RD_PTR_CHAN(x)` | `0x20*x + 0x8` |
| `MACE_AUDIO_WR_PTR_CHAN(x)` | `0x20*x + 0x10` |
| `MACE_AUDIO_RING_DEPTH_CHAN(x)` | `0x20*x + 0x18` |

PROM observations (`firmware.S`): channel 2 uses ring control `0x40`, read
pointer `0x48`, write pointer `0x50`, and depth `0x58`; ring control and write
pointer are accessed as 64-bit registers. The PROM setup writes control values
`0x1000` and `0x200`, then resets the write pointer. Codec status is polled at
`0x08` and codec input is read/written at `0x18`; codec bit definitions are not
present in the recovered PROM header.

### I2C

| Register | Offset |
|----------|--------|
| `MACE_I2C_CONFIG` | `0x00` |
| `MACE_I2C_STATUS` | `0x10` |
| `MACE_I2C_DATA` | `0x18` |

### PS/2 (keyboard + mouse)

| Register | Offset |
|----------|--------|
| `MACE_KEYBOARD_TX_BUF` | `0x00` |
| `MACE_KEYBOARD_RX_BUF` | `0x08` |
| `MACE_KEYBOARD_CONTROL` | `0x10` |
| `MACE_KEYBOARD_STATUS` | `0x18` |
| `MACE_MOUSE_TX_BUF` | `0x20` |
| `MACE_MOUSE_RX_BUF` | `0x28` |
| `MACE_MOUSE_CONTROL` | `0x30` |
| `MACE_MOUSE_STATUS` | `0x38` |

### ISA interface

| Register | Offset |
|----------|--------|
| `ISA_RING_BASE_AND_RESET` | `0x00` |
| `ISA_MISC_CONTROL` | `0x08` |

`ISA_MISC_CONTROL` bits: `ISA_RESET` `0x01`, `ISA_FLASH_ROM_WRITE_ENABLE` `1<<0`,
`ISA_RED_LED` `1<<4`, `ISA_GREEN_LED` `1<<5`.

## UART registers (16550-style, decompiled PROM `definitions.h`)

UARTs are at `0x1f390000` (UART1) and `0x1f398000` (UART2). Registers are
byte-addressed with `UART_REG(x) = (x << 8) + 7`:

| Register | `UART_REG` |
|----------|-----------|
| `UART_DATA` | `0x07` |
| `UART_IER` | `0x107` |
| `UART_IIR` | `0x207` |
| `UART_LCR` | `0x307` |
| `UART_MCR` | `0x407` |
| `UART_LSR` | `0x507` |
| `UART_MSR` | `0x607` |
| `UART_SCR` | `0x707` |

## RTC registers (MC146818-style, decompiled PROM `definitions.h`)

RTC is at `0x1f3a0000`. Registers are byte-addressed with `RTC_REG(x) = x << 8`:

| Register | `RTC_REG` |
|----------|-----------|
| `RTC_SECONDS` | `0x00` |
| `RTC_SECONDS_ALARM` | `0x100` |
| `RTC_MINUTES` | `0x200` |
| `RTC_MINUTES_ALARM` | `0x300` |
| `RTC_HOURS` | `0x400` |
| `RTC_HOURS_ALARM` | `0x500` |
| `RTC_DAY_OF_WEEK` | `0x600` |
| `RTC_DAY_OF_MONTH` | `0x700` |
| `RTC_MONTH` | `0x800` |
| `RTC_YEAR` | `0x900` |
| `RTC_CTRL_A/B/C/D` | `0xa00`–`0xd00` |
| `RTC_CRC` | `0x4700` |
| `RTC_CENTURY` | `0x4800` |
| `RTC_DATE_ALARM` | `0x4900` |
| `RTC_EXT_CTRL_4A/4B` | `0x4a00`/`0x4b00` |
| `RTC_NVRAM(x)` | `(0x0e + x) << 8` |

## TODO / Open questions

- [ ] ICE ASIC register map (no authoritative register header located yet)
- [x] PROM-visible MRE/RE/DE/MTE register map (`definitions.h`, base
    `0x15000000`)
- [x] Display Engine / GBE register map (`include/video/gbe.h`)
- [ ] Cross-check the GBE map against the leaked IRIX `crmGfxState.c`
- [x] PROM-level MACE Ethernet register map
- [x] PROM-level MACE audio ring/codec register map
- [ ] Complete Ethernet packet/descriptor register map
- [ ] Complete mavb codec and audio register semantics