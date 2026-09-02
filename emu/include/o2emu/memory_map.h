#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace o2emu {

// Physical address map (from IRIX and PROM)
constexpr uint32_t PHYS_CRIME_BASE = 0x14000000;
constexpr uint32_t PHYS_RE_BASE = 0x15000000;
constexpr uint32_t PHYS_GBE_BASE = 0x16000000;
constexpr uint32_t PHYS_MACE_BASE = 0x1f000000;
constexpr uint32_t PHYS_PROM_BASE = 0x1fc00000;

// CRIME CPU Interface registers (base 0x14000000)
enum class CrimeCpuReg : uint32_t {
  ID = 0x0000,
  CONTROL = 0x0008,
  INTSTAT = 0x0010,
  INTMASK = 0x0018,
  SOFTINT = 0x0020,
  HARDINT = 0x0028,
  DOG = 0x0030,
  TIME = 0x0038,
  CPU_ERROR_ADDR = 0x0040,
  CPU_ERROR_STAT = 0x0048,
  CPU_ERROR_ENA = 0x0050,
  VICE_ERROR_ADDR = 0x0058,
  MEM_CONTROL = 0x0200,
  MEM_BANK_CTRL_0 = 0x0208,
  MEM_BANK_CTRL_1 = 0x0210,
  MEM_BANK_CTRL_2 = 0x0218,
  MEM_BANK_CTRL_3 = 0x0220,
  MEM_BANK_CTRL_4 = 0x0228,
  MEM_BANK_CTRL_5 = 0x0230,
  MEM_BANK_CTRL_6 = 0x0238,
  MEM_BANK_CTRL_7 = 0x0240,
  MEM_REFRESH_CNTR = 0x0248,
  MEM_ERROR_STAT = 0x0250,
  MEM_ERROR_ADDR = 0x0258,
  MEM_ERROR_ECC_SYN = 0x0260,
  MEM_ERROR_ECC_CHK = 0x0268,
  MEM_ERROR_ECC_REPL = 0x0270,
};

// CRIME Control register bits
constexpr uint32_t CRM_CTRL_TRITON_SYSADC = 0x2000;
constexpr uint32_t CRM_CTRL_CRIME_SYSADC = 0x1000;
constexpr uint32_t CRM_CTRL_HARD_RESET = 0x0800;
constexpr uint32_t CRM_CTRL_SOFT_RESET = 0x0400;
constexpr uint32_t CRM_CTRL_DOG_ENA = 0x0200;
constexpr uint32_t CRM_CTRL_ENDIANESS = 0x0100;
constexpr uint32_t CRM_CTRL_ENDIAN_BIG = 0x0100;
constexpr uint32_t CRM_CTRL_ENDIAN_LITTLE = 0x0000;
constexpr uint32_t CRM_CTRL_CQUEUE_HWM_MASK = 0x000f;
constexpr uint32_t CRM_CTRL_CQUEUE_HWM_SHIFT = 0;
constexpr uint32_t CRM_CTRL_WBUF_HWM_MASK = 0x00f0;
constexpr uint32_t CRM_CTRL_WBUF_HWM_SHIFT = 8;

// CRIME interrupt bits
constexpr uint32_t CRM_INT_VICE = 0x80000000;
constexpr uint32_t CRM_INT_SOFT2 = 0x40000000;
constexpr uint32_t CRM_INT_SOFT1 = 0x20000000;
constexpr uint32_t CRM_INT_SOFT0 = 0x10000000;
constexpr uint32_t CRM_INT_RE5 = 0x08000000;
constexpr uint32_t CRM_INT_RE4 = 0x04000000;
constexpr uint32_t CRM_INT_RE3 = 0x02000000;
constexpr uint32_t CRM_INT_RE2 = 0x01000000;
constexpr uint32_t CRM_INT_RE1 = 0x00800000;
constexpr uint32_t CRM_INT_RE0 = 0x00400000;
constexpr uint32_t CRM_INT_MEMERR = 0x00200000;
constexpr uint32_t CRM_INT_CRMERR = 0x00100000;
constexpr uint32_t CRM_INT_GBE3 = 0x00080000;
constexpr uint32_t CRM_INT_GBE2 = 0x00040000;
constexpr uint32_t CRM_INT_GBE1 = 0x00020000;
constexpr uint32_t CRM_INT_GBE0 = 0x00010000;
constexpr uint32_t CRM_INT_MACE_BASE = 0x00000001; // bits 0-15

// CRIME Render Engine registers (base 0x15000000)
enum class CrimeRePage : uint32_t {
  INTF_BUF = 0x0000,
  TLB = 0x1000,
  PIX_PIPE = 0x2000,
  MTE = 0x3000,
  STATUS = 0x4000,
};

// RE Interface Buffer (page 0)
enum class ReIntfBufReg : uint32_t {
  DATA_BASE = 0x000, // 64 entries × 8 bytes
  ADDR_BASE = 0x200, // 64 entries × 8 bytes
  CTL = 0x400,
  RESET = 0x408,
};

// RE TLB (page 1)
enum class ReTlbReg : uint32_t {
  FB_A_BASE = 0x000, // 64 entries
  FB_B_BASE = 0x200,
  FB_C_BASE = 0x400,
  TEX_BASE = 0x600,      // 28 entries
  CID_BASE = 0x6e0,      // 4 entries
  LINEAR_A_BASE = 0x700, // 16 entries
  LINEAR_B_BASE = 0x780, // 16 entries
};

// RE Pixel Pipe / Draw (page 2)
enum class RePixPipeReg : uint32_t {
  BUF_MODE_SRC = 0x000,
  BUF_MODE_DST = 0x008,
  CLIP_MODE = 0x010,
  DRAW_MODE = 0x018,
  SCR_MASK_0 = 0x020,
  SCR_MASK_1 = 0x028,
  SCR_MASK_2 = 0x030,
  SCR_MASK_3 = 0x038,
  SCR_MASK_4 = 0x040,
  SCISSOR = 0x048,
  WIN_OFFSET_SRC = 0x050,
  WIN_OFFSET_DST = 0x058,
  PRIMITIVE = 0x060,
  VERTEX_X_0 = 0x070,
  VERTEX_X_1 = 0x074,
  VERTEX_X_2 = 0x078,
  VERTEX_GL_X0 = 0x080,
  VERTEX_GL_Y0 = 0x084,
  VERTEX_GL_X1 = 0x088,
  VERTEX_GL_Y1 = 0x08c,
  VERTEX_GL_X2 = 0x090,
  VERTEX_GL_Y2 = 0x094,
  START_SETUP = 0x098,
  PIXEL_XFER_SRC = 0x0a0,
  PIXEL_XFER_DST = 0x0b0,
  STIPPLE = 0x0c0,
  SHADE_BASE = 0x0d0,   // 12 registers
  TEXTURE_BASE = 0x110, // 23 registers
  FOG_BASE = 0x170,
  ANTIALIAS_BASE = 0x190,
  ALPHA_TEST = 0x198,
  BLEND_BASE = 0x1a0,
  LOGIC_OP = 0x1b0,
  COLOR_MASK = 0x1b8,
  DEPTH_BASE = 0x1c0,
  STENCIL_BASE = 0x1e0,
  PIX_PIPE_NULL = 0x1f0,
  PIX_PIPE_FLUSH = 0x1f8,
};

// RE MTE (page 3)
enum class ReMteReg : uint32_t {
  MODE = 0x00,
  BYTE_MASK = 0x08,
  STIPPLE_MASK = 0x10,
  FG_VALUE = 0x18,
  SRC0 = 0x20,
  SRC1 = 0x28,
  DST0 = 0x30,
  DST1 = 0x38,
  SRC_Y_STEP = 0x40,
  DST_Y_STEP = 0x48,
  NULL_REG = 0x70,
  FLUSH = 0x78,
};

// RE Status (page 4)
enum class ReStatusReg : uint32_t {
  STATUS = 0x00,
  SET_START_PTR = 0x08,
};

// Buffer mode bitfields (from IRIX crimedef.h)
constexpr uint32_t BM_DOUBLE_PIX_SEL = 1u << 0;
constexpr uint32_t BM_DOUBLE_PIX = 1u << 1;
constexpr uint32_t BM_PIX_DEPTH_SHIFT = 2;
constexpr uint32_t BM_PIX_DEPTH_MASK = 3u << BM_PIX_DEPTH_SHIFT;
constexpr uint32_t BM_PIX_TYPE_SHIFT = 4;
constexpr uint32_t BM_PIX_TYPE_MASK = 3u << BM_PIX_TYPE_SHIFT;
constexpr uint32_t BM_BUF_DEPTH_SHIFT = 8;
constexpr uint32_t BM_BUF_DEPTH_MASK = 3u << BM_BUF_DEPTH_SHIFT;
constexpr uint32_t BM_BUF_TYPE_SHIFT = 10;
constexpr uint32_t BM_BUF_TYPE_MASK = 7u << BM_BUF_TYPE_SHIFT;

enum class BufType : uint32_t {
  FB_A = 0,
  FB_B = 1,
  FB_C = 2,
  TEX = 3,
  LINEAR_A = 4,
  LINEAR_B = 5,
  CID = 6,
};

// Draw mode bitfields
constexpr uint32_t DM_ENNOCONFLICT = 1u << 23;
constexpr uint32_t DM_ENGL = 1u << 22;
constexpr uint32_t DM_ENPIXXFER = 1u << 21;
constexpr uint32_t DM_ENSCISSORTEST = 1u << 20;
constexpr uint32_t DM_ENLINESTIPPLE = 1u << 19;
constexpr uint32_t DM_ENPOLYSTIPPLE = 1u << 18;
constexpr uint32_t DM_ENOPAQSTIPPLE = 1u << 17;
constexpr uint32_t DM_ENSMOOTHSHADE = 1u << 16;
constexpr uint32_t DM_ENTEXTURE = 1u << 15;
constexpr uint32_t DM_ENFOG = 1u << 14;
constexpr uint32_t DM_ENCOVERAGE = 1u << 13;
constexpr uint32_t DM_ENANTILINE = 1u << 12;
constexpr uint32_t DM_ENALPHATEST = 1u << 11;
constexpr uint32_t DM_ENBLEND = 1u << 10;
constexpr uint32_t DM_ENLOGICOP = 1u << 9;
constexpr uint32_t DM_ENDITHER = 1u << 8;
constexpr uint32_t DM_ENCOLORMASK = 1u << 7;
constexpr uint32_t DM_ENCOLORBYTEMASK = 0x78; // bits 3-6
constexpr uint32_t DM_ENDEPTHTEST = 1u << 2;
constexpr uint32_t DM_ENDEPTHMASK = 1u << 1;
constexpr uint32_t DM_ENSTENCILTEST = 1u << 0;

// GBE Display Engine registers (base 0x16000000)
enum class GbeReg : uint32_t {
  // Control / clock / ID (0x00000)
  CTRLSTAT = 0x00000,
  DOTCLOCK = 0x00004,
  I2C = 0x00008,
  SYSCLK = 0x0000c,
  I2CFP = 0x00010,
  ID = 0x00014,

  // Video timing (0x10000)
  VT_XY = 0x10000,
  VT_XYMAX = 0x10004,
  VT_VSYNC = 0x10008,
  VT_HSYNC = 0x1000c,
  VT_VBLANK = 0x10010,
  VT_HBLANK = 0x10014,
  VT_FLAGS = 0x10018,
  VT_F2RF_LOCK = 0x1001c,
  VT_INTR01 = 0x10020,
  VT_INTR23 = 0x10024,
  FP_HDRV = 0x10028,
  FP_VDRV = 0x1002c,
  FP_DE = 0x10030,
  VT_HPIXEN = 0x10034,
  VT_VPIXEN = 0x10038,
  VT_HCMAP = 0x1003c,
  VT_VCMAP = 0x10040,
  DID_START_XY = 0x10044,
  CRS_START_XY = 0x10048,
  VC_START_XY = 0x1004c,

  // Overlay (0x20000)
  OVR_WIDTH_TILE = 0x20000,
  OVR_CONTROL = 0x20004,

  // Framebuffer (0x30000)
  FRM_SIZE_TILE = 0x30000,
  FRM_SIZE_PIXEL = 0x30004,
  FRM_CONTROL = 0x30008,

  // DID (0x40000)
  DID_CONTROL = 0x40000,

  // WID table (0x48000)
  MODE_REGS_BASE = 0x48000, // 32 registers

  // Colormap (0x50000)
  CMAP_BASE = 0x50000, // 4608 entries
  CM_FIFO = 0x58000,

  // Gamma (0x60000)
  GMAP_BASE = 0x60000, // 256 entries

  // Cursor (0x70000)
  CRS_POS = 0x70000,
  CRS_CTL = 0x70004,
  CRS_CMAP_BASE = 0x70008,  // 3 entries
  CRS_GLYPH_BASE = 0x78000, // 64 entries

  // Video capture (0x80000)
  VC_LR = 0x80000,
  VC_TB = 0x80004,
  VC_FILTERS = 0x80008,
  VC_CONTROL = 0x8000c,
};

// GBE mode register bitfields
constexpr uint32_t GBE_WID_BUF_MASK = 0x3;
constexpr uint32_t GBE_WID_TYPE_MASK = 0x1C;
constexpr uint32_t GBE_WID_TYPE_SHIFT = 2;
constexpr uint32_t GBE_WID_CM_MASK = 0x3E0;
constexpr uint32_t GBE_WID_CM_SHIFT = 5;
constexpr uint32_t GBE_WID_GM_MASK = 0x400;
constexpr uint32_t GBE_WID_GM_SHIFT = 10;

enum class GbeColorMode : uint32_t {
  I8 = 0,
  I12 = 1,
  RG3B2 = 2,
  RGB4 = 3,
  RGB5 = 4,
  RGB8 = 5,
};

// MACE registers (base 0x1f000000)
enum class MaceReg : uint32_t {
  // PCI (0x080000)
  PCI_ERROR_ADDR = 0x080000,
  PCI_ERROR_FLAGS = 0x080004,
  PCI_CONTROL = 0x080008,
  PCI_REV_INFO_R = 0x08000c,
  PCI_FLUSH_W = 0x08000c,
  PCI_CONFIG_ADDR = 0x080cf8,
  PCI_CONFIG_DATA = 0x080cfc,

  // Ethernet (0x280000)
  ETH_BASE = 0x280000,

  // Peripheral (0x300000)
  PERIF_BASE = 0x300000,
  AUDIO_BASE = 0x300000,
  ISA_BASE = 0x310000,
  KBDMS_BASE = 0x320000,
  I2C_BASE = 0x330000,
  UST_MSC_BASE = 0x340000,

  // ISA External (0x380000)
  ISA_EXT_BASE = 0x380000,
  SER1_BASE = 0x390000,
  SER2_BASE = 0x398000,
  RTC_BASE = 0x3a0000,
};

// MACE PCI Error Flags
constexpr uint32_t PERR_MASTER_ABORT = 0x80000000;
constexpr uint32_t PERR_TARGET_ABORT = 0x40000000;
constexpr uint32_t PERR_DATA_PARITY_ERR = 0x20000000;
constexpr uint32_t PERR_RETRY_ERR = 0x10000000;
constexpr uint32_t PERR_ILLEGAL_CMD = 0x08000000;
constexpr uint32_t PERR_SYSTEM_ERR = 0x04000000;
constexpr uint32_t PERR_INTERRUPT_TEST = 0x02000000;
constexpr uint32_t PERR_PARITY_ERR = 0x01000000;
constexpr uint32_t PERR_OVERRUN = 0x00800000;
constexpr uint32_t PERR_MASTER_ABORT_ADDR_VALID = 0x00080000;
constexpr uint32_t PERR_TARGET_ABORT_ADDR_VALID = 0x00040000;
constexpr uint32_t PERR_DATA_PARITY_ADDR_VALID = 0x00020000;
constexpr uint32_t PERR_RETRY_ADDR_VALID = 0x00010000;

// MACE Audio registers (relative to AUDIO_BASE)
enum class MaceAudioReg : uint32_t {
  STATUS = 0x00,
  CODEC_STATUS = 0x08,
  CODEC_INPUT_MASK = 0x10,
  CODEC_INPUT = 0x18,
  RING_CTRL_CHAN_0 = 0x00,
  RD_PTR_CHAN_0 = 0x08,
  WR_PTR_CHAN_0 = 0x10,
  RING_DEPTH_CHAN_0 = 0x18,
  RING_CTRL_CHAN_1 = 0x20,
  RD_PTR_CHAN_1 = 0x28,
  WR_PTR_CHAN_1 = 0x30,
  RING_DEPTH_CHAN_1 = 0x38,
  RING_CTRL_CHAN_2 = 0x40,
  RD_PTR_CHAN_2 = 0x48,
  WR_PTR_CHAN_2 = 0x50,
  RING_DEPTH_CHAN_2 = 0x58,
  RING_CTRL_CHAN_3 = 0x60,
  RD_PTR_CHAN_3 = 0x68,
  WR_PTR_CHAN_3 = 0x70,
  RING_DEPTH_CHAN_3 = 0x78,
};

// MACE I2C registers (relative to I2C_BASE)
enum class MaceI2cReg : uint32_t {
  CONFIG = 0x00,
  STATUS = 0x10,
  DATA = 0x18,
};

// MACE PS/2 registers (relative to KBDMS_BASE)
enum class MacePs2Reg : uint32_t {
  KBD_TX_BUF = 0x00,
  KBD_RX_BUF = 0x08,
  KBD_CONTROL = 0x10,
  KBD_STATUS = 0x18,
  MOUSE_TX_BUF = 0x20,
  MOUSE_RX_BUF = 0x28,
  MOUSE_CONTROL = 0x30,
  MOUSE_STATUS = 0x38,
};

// MACE ISA registers (relative to ISA_BASE)
enum class MaceIsaReg : uint32_t {
  RINGBASE = 0x00,
  FLASH_NIC_REG = 0x08,
  INT_STS_REG = 0x10,
  INT_MSK_REG = 0x18,
};

// ISA misc control bits
constexpr uint32_t ISA_FLASH_WE = 0x01;
constexpr uint32_t ISA_PWD_CLEAR = 0x02;
constexpr uint32_t ISA_NIC_DEASSERT = 0x04;
constexpr uint32_t ISA_NIC_DATA = 0x08;
constexpr uint32_t ISA_LED_RED = 0x10;
constexpr uint32_t ISA_LED_GREEN = 0x20;
constexpr uint32_t ISA_DP_RAM_ENABLE = 0x40;

// MACE UST/MSC registers (relative to UST_MSC_BASE)
enum class MaceUstMscReg : uint32_t {
  UST = 0x00,
  COMPARE1 = 0x08,
  COMPARE2 = 0x10,
  COMPARE3 = 0x18,
  AIN_MSC_UST = 0x20,
  AOUT1_MSC_UST = 0x28,
  AOUT2_MSC_UST = 0x30,
  VIN1_MSC_UST = 0x38,
  VIN2_MSC_UST = 0x40,
  VOUT_MSC_UST = 0x48,
};

// MACE interrupt assignments
enum class MaceIntr : uint32_t {
  VID_IN_1 = 0,
  VID_IN_2 = 1,
  VID_OUT = 2,
  ETHERNET = 3,
  PERIPH_SERIAL = 4,
  PERIPH_PARALLEL = 4,
  PERIPH_MISC = 5,
  PERIPH_AUDIO = 6,
  PCI_BRIDGE = 7,
  PCI_SCSI0 = 8,
  PCI_SCSI1 = 9,
  PCI_SLOT0 = 10,
  PCI_SLOT1 = 11,
  PCI_SLOT2 = 12,
  PCI_SHARED0 = 13,
  PCI_SHARED1 = 14,
  PCI_SHARED2 = 15,
};

// UART registers (16550-style, byte-addressed with UART_REG(x) = (x << 8) + 7)
constexpr uint32_t UART1_BASE = 0x1f390000;
constexpr uint32_t UART2_BASE = 0x1f398000;

enum class UartReg : uint32_t {
  DATA = 0x07,
  IER = 0x107,
  IIR = 0x207,
  LCR = 0x307,
  MCR = 0x407,
  LSR = 0x507,
  MSR = 0x607,
  SCR = 0x707,
};

// RTC registers (MC146818-style, byte-addressed with RTC_REG(x) = x << 8)
constexpr uint32_t RTC_BASE = 0x1f3a0000;

enum class RtcReg : uint32_t {
  SECONDS = 0x0000,
  SECONDS_ALARM = 0x0100,
  MINUTES = 0x0200,
  MINUTES_ALARM = 0x0300,
  HOURS = 0x0400,
  HOURS_ALARM = 0x0500,
  DAY_OF_WEEK = 0x0600,
  DAY_OF_MONTH = 0x0700,
  MONTH = 0x0800,
  YEAR = 0x0900,
  CTRL_A = 0x0a00,
  CTRL_B = 0x0b00,
  CTRL_C = 0x0c00,
  CTRL_D = 0x0d00,
  CRC = 0x4700,
  CENTURY = 0x4800,
  DATE_ALARM = 0x4900,
  EXT_CTRL_4A = 0x4a00,
  EXT_CTRL_4B = 0x4b00,
};

// Memory map interface
class MemoryMap {
public:
  virtual ~MemoryMap() = default;

  // Read/write 8/16/32/64-bit
  virtual uint8_t read8(uint32_t addr) = 0;
  virtual uint16_t read16(uint32_t addr) = 0;
  virtual uint32_t read32(uint32_t addr) = 0;
  virtual uint64_t read64(uint32_t addr) = 0;

  virtual void write8(uint32_t addr, uint8_t val) = 0;
  virtual void write16(uint32_t addr, uint16_t val) = 0;
  virtual void write32(uint32_t addr, uint32_t val) = 0;
  virtual void write64(uint32_t addr, uint64_t val) = 0;

  // Device access helpers
  uint32_t crime_cpu_read(uint32_t offset) {
    return read32(PHYS_CRIME_BASE + offset);
  }
  void crime_cpu_write(uint32_t offset, uint32_t val) {
    write32(PHYS_CRIME_BASE + offset, val);
  }

  uint32_t crime_re_read(uint32_t page, uint32_t offset) {
    return read32(PHYS_RE_BASE + page + offset);
  }
  void crime_re_write(uint32_t page, uint32_t offset, uint32_t val) {
    write32(PHYS_RE_BASE + page + offset, val);
  }

  uint32_t gbe_read(uint32_t offset) { return read32(PHYS_GBE_BASE + offset); }
  void gbe_write(uint32_t offset, uint32_t val) {
    write32(PHYS_GBE_BASE + offset, val);
  }

  uint32_t mace_read(uint32_t offset) {
    return read32(PHYS_MACE_BASE + offset);
  }
  void mace_write(uint32_t offset, uint32_t val) {
    write32(PHYS_MACE_BASE + offset, val);
  }
};

} // namespace o2emu