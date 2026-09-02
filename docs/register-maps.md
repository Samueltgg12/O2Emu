# O2 (IP32) Register Maps — from Linux/NetBSD drivers and IRIX source

Register-level details mined from open-source drivers and the leaked IRIX 6.5.17
source tree. These are the ground truth for emulator implementation.

## Memory Map (IRIX `irix/kern/sys/mace.h`, `irix/kern/sys/crime.h`)

| Base | Device |
|------|--------|
| `0x14000000` | CRIME CPU Interface (`CRM_BASEADDR`) |
| `0x15000000` | CRIME Render Engine (`CRM_RE_BASE_ADDRESS`) |
| `0x16000000` | GBE Display Engine (`GBECHIP_ADDR`) |
| `0x1f000000` | MACE (`MACE_BASE`) |
| `0x1fc00000` | System ROM / PROM |

### MACE sub-offsets (relative to `0x1f000000`, IRIX `mace.h`)

| Offset | Device |
|--------|--------|
| `0x080000` | PCI (`MACE_PCI`) |
| `0x100000` | Video In 1 (`MACE_VIN1`) |
| `0x180000` | Video In 2 (`MACE_VIN2`) |
| `0x200000` | Video Out (`MACE_VOUT`) |
| `0x280000` | Ethernet (`MACE_ENET`) |
| `0x300000` | Peripheral (`MACE_PERIF`) |
| `0x380000` | ISA External (`MACE_ISA_EXT`) |

### Peripheral sub-offsets (relative to `MACE_PERIF`)

| Offset | Device |
|--------|--------|
| `0x00000` | Audio (`MACE_AUDIO`) |
| `0x10000` | ISA (`MACE_ISA`) |
| `0x20000` | Keyboard/Mouse (`MACE_KBDMS`) |
| `0x30000` | I2C (`MACE_I2C`) |
| `0x40000` | UST/MSC (`MACE_UST_MSC`) |

### ISA External sub-offsets (relative to `MACE_ISA_EXT`)

| Offset | Device |
|--------|--------|
| `0x00000` | EPP (`ISA_EPP_BASE`) |
| `0x08000` | ECP (`ISA_ECP_BASE`) |
| `0x10000` | Serial 1 (`ISA_SER1_BASE`) |
| `0x18000` | Serial 2 (`ISA_SER2_BASE`) |
| `0x20000` | RTC (`ISA_RTC_BASE`) |
| `0x30000` | Game port (`ISA_GAME_BASE`) |

## CRIME ASIC (IRIX `irix/kern/sys/crime.h`, `stand/arcs/IP32prom/include/sys/crimereg.h`, `crimedef.h`, `crimechip.h`)

CRIME is the memory controller / system controller ASIC. It contains:
- CPU Interface (memory controller, interrupt controller, timers)
- Render Engine (RE) — 3D pipeline, TLB, pixel pipe, MTE
- GBE Display Engine interface

### CRIME CPU Interface registers (base `0x14000000`, IRIX `crime.h`)

| Register | Offset | Mask | Description |
|----------|--------|------|-------------|
| `CRM_ID` | `0x00` | `0xff` | Chip ID/revision |
| `CRM_CONTROL` | `0x08` | `0x3fff` | Control register |
| `CRM_INTSTAT` | `0x10` | `0xffffffff` | Interrupt status |
| `CRM_INTMASK` | `0x18` | `0xffffffff` | Interrupt mask |
| `CRM_SOFTINT` | `0x20` | `0xffffffff` | Software interrupt |
| `CRM_HARDINT` | `0x28` | `0xf0ffffff` | Hardware interrupt |
| `CRM_DOG` | `0x30` | `0x1fffff` | Watchdog timer |
| `CRM_TIME` | `0x38` | `0xffffffffffff` | Crime timer (66.6 MHz) |
| `CRM_CPU_ERROR_ADDR` | `0x40` | `0x3ffffffff` | CPU error address |
| `CRM_CPU_ERROR_STAT` | `0x48` | `0x7` | CPU error status |
| `CRM_CPU_ERROR_ENA` | `0x50` | `0x7` | CPU error enable |
| `CRM_VICE_ERROR_ADDR` | `0x58` | `0x3fffffff` | VICE (ICE) error address |
| `CRM_MEM_CONTROL` | `0x200` | `0x3` | Memory control (ECC) |
| `CRM_MEM_BANK_CTRL(x)` | `0x208 + x*8` | `0x11f` | Bank control (8 banks) |
| `CRM_MEM_REFRESH_CNTR` | `0x248` | `0x7ff` | Refresh counter |
| `CRM_MEM_ERROR_STAT` | `0x250` | `0x0ff7ffff` | Memory error status |
| `CRM_MEM_ERROR_ADDR` | `0x258` | `0x3fffffff` | Memory error address |
| `CRM_MEM_ERROR_ECC_SYN` | `0x260` | `0xffffffff` | ECC syndrome |
| `CRM_MEM_ERROR_ECC_CHK` | `0x268` | `0xffffffff` | ECC check bits |
| `CRM_MEM_ERROR_ECC_REPL` | `0x270` | `0xffffffff` | ECC replacement bits |

### CRIME Control register bits (IRIX `crime.h`)

```text
CRM_CONTROL_TRITON_SYSADC       0x2000
CRM_CONTROL_CRIME_SYSADC        0x1000
CRM_CONTROL_HARD_RESET          0x0800
CRM_CONTROL_SOFT_RESET          0x0400
CRM_CONTROL_DOG_ENA             0x0200
CRM_CONTROL_ENDIANESS           0x0100
CRM_CONTROL_ENDIAN_BIG          0x0100
CRM_CONTROL_ENDIAN_LITTLE       0x0000
CRM_CONTROL_CQUEUE_HWM          0x000f   (command queue high-water mark)
CRM_CONTROL_CQUEUE_SHFT         0
CRM_CONTROL_WBUF_HWM            0x00f0   (write buffer high-water mark)
CRM_CONTROL_WBUF_SHFT           8
```

### CRIME interrupt bits (IRIX `crime.h`)

```text
CRM_INT_VICE        0x80000000  (VICE/ICE interrupt)
CRM_INT_SOFT2       0x40000000
CRM_INT_SOFT1       0x20000000
CRM_INT_SOFT0       0x10000000
CRM_INT_RE5         0x08000000
CRM_INT_RE4         0x04000000
CRM_INT_RE3         0x02000000
CRM_INT_RE2         0x01000000
CRM_INT_RE1         0x00800000
CRM_INT_RE0         0x00400000
CRM_INT_MEMERR      0x00200000
CRM_INT_CRMERR      0x00100000
CRM_INT_GBE3        0x00080000
CRM_INT_GBE2        0x00040000
CRM_INT_GBE1        0x00020000
CRM_INT_GBE0        0x00010000
CRM_INT_GBEx        (CRM_INT_GBE0|CRM_INT_GBE1|CRM_INT_GBE2|CRM_INT_GBE3)
CRM_INT_MACE(i)     (1 << i)   (i = 0..15)
```

### CRIME CPU error status bits (IRIX `crime.h`)

```text
CRM_CPU_ERROR_CPU_ILL_ADDR      0x4
CRM_CPU_ERROR_VICE_WRT_PRTY     0x2
CRM_CPU_ERROR_CPU_WRT_PRTY      0x1
```

### CRIME Memory error status bits (IRIX `crime.h`)

```text
CRM_MEM_ERROR_MACE_ID           0x0000007f
CRM_MEM_ERROR_MACE_ACCESS       0x00000080
CRM_MEM_ERROR_RE_ID             0x00007f00
CRM_MEM_ERROR_RE_ACCESS         0x00008000
CRM_MEM_ERROR_GBE_ACCESS        0x00010000
CRM_MEM_ERROR_VICE_ACCESS       0x00020000
CRM_MEM_ERROR_CPU_ACCESS        0x00040000
CRM_MEM_ERROR_SOFT_ERR          0x00100000
CRM_MEM_ERROR_HARD_ERR          0x00200000
CRM_MEM_ERROR_MULTIPLE          0x00400000
CRM_MEM_ERROR_MEM_ECC_RD        0x00800000
CRM_MEM_ERROR_MEM_ECC_RMW       0x01000000
CRM_MEM_ERROR_INV_MEM_ADDR_RD   0x02000000
CRM_MEM_ERROR_INV_MEM_ADDR_WR   0x04000000
CRM_MEM_ERROR_INV_MEM_ADDR_RMW  0x08000000
```

## CRIME Render Engine (RE) registers (base `0x15000000`, IRIX `crimereg.h`, `crimedef.h`, `crimechip.h`)

The RE is organized in 4KB pages:

| Page | Base Offset | Contents |
|------|-------------|----------|
| 0 | `0x0000` | Interface Buffer (`CrmIntfBufReg`) |
| 1 | `0x1000` | TLB (`CrmTlbReg`) |
| 2 | `0x2000` | Pixel Pipe / Draw registers |
| 3 | `0x3000` | MTE (Memory Transfer Engine) |
| 4 | `0x4000` | Status / SetStartPtr |

### Interface Buffer (page 0, `CRM_INTFBUF_BASE = 0x0`)

| Register | Offset | Description |
|----------|--------|-------------|
| `data[64]` | `0x000` | FIFO data (2 words each) |
| `addr[64]` | `0x200` | FIFO address |
| `ctl` | `0x400` | Control (full/empty/stall levels) |
| `reset` | `0x408` | Reset |

### TLB (page 1, `CRM_TLB_BASE = 0x1000`)

| TLB | Offset | Entries |
|-----|--------|---------|
| FB A | `0x000` | 64 |
| FB B | `0x200` | 64 |
| FB C | `0x400` | 64 |
| Texture | `0x600` | 28 |
| CID | `0x6e0` | 4 |
| Linear A | `0x700` | 16 |
| Linear B | `0x780` | 16 |

Each TLB entry is 8 bytes (2×32-bit).

### Pixel Pipe / Draw registers (page 2, `CRM_PIXPIPE_BASE = 0x2000`)

| Register | Offset | Description |
|----------|--------|-------------|
| `BufMode` (src) | `0x000` | Source buffer mode |
| `BufMode` (dst) | `0x008` | Destination buffer mode |
| `ClipMode` | `0x010` | Clip mode |
| `DrawMode` | `0x018` | Draw mode |
| `ScrMask[5]` | `0x020`–`0x040` | Screen masks |
| `Scissor` | `0x048` | Scissor rectangle |
| `WinOffset` (src) | `0x050` | Window offset source |
| `WinOffset` (dst) | `0x058` | Window offset dest |
| `Primitive` | `0x060` | Primitive type/width |
| `Vertex X[3]` | `0x070`–`0x078` | Vertex X coordinates |
| `Vertex GL[3]` | `0x080`–`0x094` | Vertex GL (x,y) |
| `StartSetup` | `0x098` | Start setup |
| `PixelXfer` (src) | `0x0a0` | Pixel transfer source |
| `PixelXfer` (dst) | `0x0b0` | Pixel transfer dest |
| `Stipple` | `0x0c0` | Stipple mode/pattern |
| `Shade` | `0x0d0` | Shade registers (12) |
| `Texture` | `0x110` | Texture registers (23) |
| `Fog` | `0x170` | Fog registers |
| `Antialias` | `0x190` | Antialias line/coverage |
| `AlphaTest` | `0x198` | Alpha test |
| `Blend` | `0x1a0` | Blend constant/function |
| `LogicOp` | `0x1b0` | Logic operation |
| `ColorMask` | `0x1b8` | Color mask |
| `Depth` | `0x1c0` | Depth func/z0/dzdx/dzdy |
| `Stencil` | `0x1e0` | Stencil mode/mask |
| `PixPipeNull` | `0x1f0` | Null register |
| `PixPipeFlush` | `0x1f8` | Flush pipeline |

### MTE (page 3, `CRM_MTE_BASE = 0x3000`)

| Register | Offset | Description |
|----------|--------|-------------|
| `MTE_MODE` | `0x00` | Mode (clear/copy, stipple, depth) |
| `MTE_BYTEMASK` | `0x08` | Byte mask |
| `MTE_STIPPLEMASK` | `0x10` | Stipple mask |
| `MTE_FGVALUE` | `0x18` | Foreground value |
| `MTE_SRC0` | `0x20` | Source 0 |
| `MTE_SRC1` | `0x28` | Source 1 |
| `MTE_DST0` | `0x30` | Destination 0 |
| `MTE_DST1` | `0x38` | Destination 1 |
| `MTE_SRCYSTEP` | `0x40` | Source Y step |
| `MTE_DSTYSTEP` | `0x48` | Destination Y step |
| `MTE_NULL` | `0x70` | Null |
| `MTE_FLUSH` | `0x78` | Flush |

### Status (page 4, `CRM_STATUS_BASE = 0x4000`)

| Register | Offset | Description |
|----------|--------|-------------|
| `Status` | `0x00` | Status register |
| `SetStartPtr` | `0x08` | Set start pointer |

### Buffer mode bitfields (IRIX `crimedef.h`)

```text
BM_DOUBLE_PIX_SEL     BIT(0)
BM_DOUBLE_PIX         BIT(1)
BM_PIX_DEPTH_SHIFT    2   (mask 3<<2)
BM_PIX_TYPE_SHIFT     4   (mask 3<<4)
BM_BUF_DEPTH_SHIFT    8   (mask 3<<8)
BM_BUF_TYPE_SHIFT     10  (mask 7<<10)
```

Buffer types: `FB_A=0`, `FB_B=1`, `FB_C=2`, `TEX=3`, `LINEAR_A=4`, `LINEAR_B=5`, `CID=6`

### Draw mode bitfields (IRIX `crimedef.h`)

```text
DM_ENNOCONFLICT     BIT(23)
DM_ENGL             BIT(22)
DM_ENPIXXFER        BIT(21)
DM_ENSCISSORTEST    BIT(20)
DM_ENLINESTIPPLE    BIT(19)
DM_ENPOLYSTIPPLE    BIT(18)
DM_ENOPAQSTIPPLE    BIT(17)
DM_ENSMOOTHSHADE    BIT(16)
DM_ENTEXTURE        BIT(15)
DM_ENFOG            BIT(14)
DM_ENCOVERAGE       BIT(13)
DM_ENANTILINE       BIT(12)
DM_ENALPHATEST      BIT(11)
DM_ENBLEND          BIT(10)
DM_ENLOGICOP        BIT(9)
DM_ENDITHER         BIT(8)
DM_ENCOLORMASK      BIT(7)
DM_ENCOLORBYTEMASK  BIT(3..6)
DM_ENDEPTHTEST      BIT(2)
DM_ENDEPTHMASK      BIT(1)
DM_ENSTENCILTEST    BIT(0)
```

## GBE (Display Engine) register map (IRIX `stand/arcs/IP32prom/include/sys/crime_gbe.h`, `gbedefs.h`)

GBE base: `0x16000000` (`GBECHIP_ADDR`). Register block is ~512KB.

### Control / clock / ID block (offset `0x00000`)

| Register | Offset | Description |
|----------|--------|-------------|
| `ctrlstat` | `0x00000` | Control/status, CHIPID 3:0 |
| `dotclock` | `0x00004` | Dot clock PLL (M 7:0, N 13:8, P 15:14, RUN 20) |
| `i2c` | `0x00008` | I2C interface |
| `sysclk` | `0x0000c` | System clock PLL |
| `i2cfp` | `0x00010` | I2C flat panel |
| `id` | `0x00014` | Device ID/revision |

### Video timing block (offset `0x10000`)

| Register | Offset | Description |
|----------|--------|-------------|
| `vt_xy` | `0x10000` | Current dot coords (X 11:0, Y 23:12, FREEZE 31) |
| `vt_xymax` | `0x10004` | Max dot coords (MAXX 11:0, MAXY 23:12) |
| `vt_vsync` | `0x10008` | VSync on/off |
| `vt_hsync` | `0x1000c` | HSync on/off |
| `vt_vblank` | `0x10010` | VBlank on/off |
| `vt_hblank` | `0x10014` | HBlank on/off |
| `vt_flags` | `0x10018` | Polarity flags |
| `vt_f2rf_lock` | `0x1001c` | Frame-to-raster lock |
| `vt_intr01` | `0x10020` | Interrupt 0/1 Y coords |
| `vt_intr23` | `0x10024` | Interrupt 2/3 Y coords |
| `fp_hdrv` | `0x10028` | Flat panel HDRV |
| `fp_vdrv` | `0x1002c` | Flat panel VDRV |
| `fp_de` | `0x10030` | Flat panel DE |
| `vt_hpixen` | `0x10034` | Horiz pixel enable |
| `vt_vpixen` | `0x10038` | Vert pixel enable |
| `vt_hcmap` | `0x1003c` | Horiz cmap write enable |
| `vt_vcmap` | `0x10040` | Vert cmap write enable |
| `did_start_xy` | `0x10044` | DID start X/Y |
| `crs_start_xy` | `0x10048` | Cursor start X/Y |
| `vc_start_xy` | `0x1004c` | Video capture start X/Y |

### Overlay plane (offset `0x20000`)

| Register | Offset | Description |
|----------|--------|-------------|
| `ovr_width_tile` | `0x20000` | Overlay width in tiles |
| `ovr_control` | `0x20004` | Tile list ptr + DMA enable |

### Framebuffer plane (offset `0x30000`)

| Register | Offset | Description |
|----------|--------|-------------|
| `frm_size_tile` | `0x30000` | FRM_RHS 4:0, FRM_WIDTH_TILE 12:5, FRM_DEPTH 14:13 |
| `frm_size_pixel` | `0x30004` | FB_HEIGHT_PIX 31:16 |
| `frm_control` | `0x30008` | Tile list ptr + DMA enable |

### DID control (offset `0x40000`)

| Register | Offset | Description |
|----------|--------|-------------|
| `did_control` | `0x40000` | DID table ptr + DMA enable |

### WID table (offset `0x48000`)

| Register | Offset | Description |
|----------|--------|-------------|
| `mode_regs[32]` | `0x48000` | WID mode registers |

### Color / gamma map (offset `0x50000`)

| Register | Offset | Description |
|----------|--------|-------------|
| `cmap[4608]` | `0x50000` | Color palette |
| `cm_fifo` | `0x58000` | Cmap FIFO status |
| `gmap[256]` | `0x60000` | Gamma ramp |

### Cursor (offset `0x70000`)

| Register | Offset | Description |
|----------|--------|-------------|
| `crs_pos` | `0x70000` | Cursor position (X 11:0, Y 27:16) |
| `crs_ctl` | `0x70004` | Cursor enable, crosshair |
| `crs_cmap[3]` | `0x70008` | Cursor colors 1-3 |
| `crs_glyph[64]` | `0x78000` | Cursor glyph |

### Video capture (offset `0x80000`)

| Register | Offset | Description |
|----------|--------|-------------|
| `vc_lr` | `0x80000` | Capture window X coords |
| `vc_tb` | `0x80004` | Capture window Y coords |
| `vc_filters` | `0x80008` | Capture filters |
| `vc_control` | `0x8000c` | Capture control |

### GBE mode register bitfields (IRIX `gbedefs.h`)

```text
GBE_WID_BUF_MASK      0x3
GBE_WID_TYPE_MASK     0x1C  (shift 2)
GBE_WID_CM_MASK       0x3E0 (shift 5)
GBE_WID_GM_MASK       0x400 (shift 10)

GBE_CMODE_I8          0
GBE_CMODE_I12         1
GBE_CMODE_RG3B2       2
GBE_CMODE_RGB4        3
GBE_CMODE_RGB5        4
GBE_CMODE_RGB8        5
```

### Video timing tables (IRIX `crm_timing.h`)

Standard timings: 640×480@60, 800×600@60/72, 1024×768@60/70/75, 1280×1024@48/50/60/72/75, 1280×492@120 (stereo), 1600×1024@50.

## MACE ASIC (IRIX `irix/kern/sys/mace.h`)

MACE = Multimedia, Audio and Communications Engine. Base: `0x1f000000`.

### PCI host bridge (offset `MACE_PCI = 0x080000`)

| Register | Offset | Description |
|----------|--------|-------------|
| `PCI_ERROR_ADDR` | `0x00` | Error address |
| `PCI_ERROR_FLAGS` | `0x04` | Error flags |
| `PCI_CONTROL` | `0x08` | PCI control |
| `PCI_REV_INFO_R` | `0x0c` | Revision info (read) |
| `PCI_FLUSH_W` | `0x0c` | Flush (write) |
| `PCI_CONFIG_ADDR` | `0xcf8` | Config address |
| `PCI_CONFIG_DATA` | `0xcfc` | Config data |

### PCI Error Flags bits (IRIX `mace.h`)

```text
PERR_MASTER_ABORT           0x80000000
PERR_TARGET_ABORT           0x40000000
PERR_DATA_PARITY_ERR        0x20000000
PERR_RETRY_ERR              0x10000000
PERR_ILLEGAL_CMD            0x08000000
PERR_SYSTEM_ERR             0x04000000
PERR_INTERRUPT_TEST         0x02000000
PERR_PARITY_ERR             0x01000000
PERR_OVERRUN                0x00800000
PERR_MASTER_ABORT_ADDR_VALID 0x00080000
PERR_TARGET_ABORT_ADDR_VALID 0x00040000
PERR_DATA_PARITY_ADDR_VALID 0x00020000
PERR_RETRY_ADDR_VALID       0x00010000
```

### Ethernet (offset `MACE_ENET = 0x280000`)

The PROM exercises the MAC control register at offset `0x04` (low 32-bit lane),
receive FIFO at `0x104`, and byte registers for write pointer (`0x45`), read
pointer (`0x46`), and depth (`0x47`). Complete packet descriptor and transmit
register map not present in PROM.

### Audio (offset `MACE_AUDIO = 0x300000`)

| Register | Offset | Description |
|----------|--------|-------------|
| `MACE_AUDIO_STATUS` | `0x00` | Status |
| `MACE_AUDIO_CODEC_STATUS` | `0x08` | Codec status |
| `MACE_AUDIO_CODEC_INPUT_MASK` | `0x10` | Codec input mask |
| `MACE_AUDIO_CODEC_INPUT` | `0x18` | Codec input |
| `MACE_AUDIO_RING_CTRL_CHAN(x)` | `0x20*x` | Ring control |
| `MACE_AUDIO_RD_PTR_CHAN(x)` | `0x20*x + 0x8` | Read pointer |
| `MACE_AUDIO_WR_PTR_CHAN(x)` | `0x20*x + 0x10` | Write pointer |
| `MACE_AUDIO_RING_DEPTH_CHAN(x)` | `0x20*x + 0x18` | Ring depth |

PROM uses channel 2: ring control `0x40`, read ptr `0x48`, write ptr `0x50`,
depth `0x58`. Ring control and write pointer are 64-bit. Codec bit definitions
not in PROM.

### I2C (offset `MACE_I2C = 0x330000`)

| Register | Offset | Description |
|----------|--------|-------------|
| `MACE_I2C_CONFIG` | `0x00` | Config |
| `MACE_I2C_STATUS` | `0x10` | Status |
| `MACE_I2C_DATA` | `0x18` | Data |

### PS/2 Keyboard/Mouse (offset `MACE_KBDMS = 0x320000`)

| Register | Offset | Description |
|----------|--------|-------------|
| `MACE_KEYBOARD_TX_BUF` | `0x00` | Keyboard TX |
| `MACE_KEYBOARD_RX_BUF` | `0x08` | Keyboard RX |
| `MACE_KEYBOARD_CONTROL` | `0x10` | Keyboard control |
| `MACE_KEYBOARD_STATUS` | `0x18` | Keyboard status |
| `MACE_MOUSE_TX_BUF` | `0x20` | Mouse TX |
| `MACE_MOUSE_RX_BUF` | `0x28` | Mouse RX |
| `MACE_MOUSE_CONTROL` | `0x30` | Mouse control |
| `MACE_MOUSE_STATUS` | `0x38` | Mouse status |

### ISA interface (offset `MACE_ISA = 0x310000`)

| Register | Offset | Description |
|----------|--------|-------------|
| `ISA_RINGBASE` | `0x00` | Ring base + reset |
| `ISA_FLASH_NIC_REG` | `0x08` | Flash/NIC/LED/DP-RAM |
| `ISA_INT_STS_REG` | `0x10` | Interrupt status |
| `ISA_INT_MSK_REG` | `0x18` | Interrupt mask |

ISA misc control bits: `FLASH_WE=0x01`, `PWD_CLEAR=0x02`, `NIC_DEASSERT=0x04`,
`NIC_DATA=0x08`, `LED_RED=0x10`, `LED_GREEN=0x20`, `DP_RAM_ENABLE=0x40`.

### UST/MSC (offset `MACE_UST_MSC = 0x340000`)

| Register | Offset | Description |
|----------|--------|-------------|
| `MACE_UST` | `0x00` | Universal System Time (64-bit) |
| `MACE_COMPARE1` | `0x08` | Compare 1 |
| `MACE_COMPARE2` | `0x10` | Compare 2 |
| `MACE_COMPARE3` | `0x18` | Compare 3 |
| `MACE_AIN_MSC_UST` | `0x20` | Audio in MSC/UST |
| `MACE_AOUT1_MSC_UST` | `0x28` | Audio out 1 MSC/UST |
| `MACE_AOUT2_MSC_UST` | `0x30` | Audio out 2 MSC/UST |
| `MACE_VIN1_MSC_UST` | `0x38` | Video in 1 MSC/UST |
| `MACE_VIN2_MSC_UST` | `0x40` | Video in 2 MSC/UST |
| `MACE_VOUT_MSC_UST` | `0x48` | Video out MSC/UST |

UST period: 960 ns.

### MACE interrupt assignments (IRIX `mace.h`)

```text
MACE_VID_IN_1         0
MACE_VID_IN_2         1
MACE_VID_OUT          2
MACE_ETHERNET         3
MACE_PERIPH_SERIAL    4
MACE_PERIPH_PARALLEL  4
MACE_PERIPH_MISC      5
MACE_PERIPH_AUDIO     6
MACE_PCI_BRIDGE       7
MACE_PCI_SCSI0        8
MACE_PCI_SCSI1        9
MACE_PCI_SLOT0        10
MACE_PCI_SLOT1        11
MACE_PCI_SLOT2        12
MACE_PCI_SHARED0      13
MACE_PCI_SHARED1      14
MACE_PCI_SHARED2      15
```

## VICE / ICE (Image Compression Engine)

The VICE (Video Image Compression Engine) is the ICE ASIC. It is referenced in
IRIX as:
- Interrupt: `VICE_CPU_INTR = 31` (IRIX `IP32.h`)
- Error address: `CRM_VICE_ERROR_ADDR = 0x14000058` (IRIX `crime.h`)
- Error status bits in `CRM_CPU_ERROR_STAT` and `CRM_MEM_ERROR_STAT`

**No authoritative VICE/ICE register header has been located in the IRIX source
tree.** The register map remains undocumented.

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

## UART registers (16550-style, decompiled PROM `definitions.h`)

UARTs at `0x1f390000` (UART1) and `0x1f398000` (UART2). Byte-addressed with
`UART_REG(x) = (x << 8) + 7`:

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

RTC at `0x1f3a0000`. Byte-addressed with `RTC_REG(x) = x << 8`:

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

- [ ] ICE/VICE ASIC register map (no authoritative register header located)
- [ ] Complete Ethernet packet/descriptor register map
- [ ] Complete mavb codec and audio register semantics
- [ ] DIMM SPD address, EEPROM layout, and probe behavior
- [ ] PCI configuration space details for O2-specific devices
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