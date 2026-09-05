#pragma once

/**
 * @file scsicontroller.h
 * @brief SCSI Controller (AIC-7880 Ultra Wide SCSI)
 *
 * Based on IRIX adp78.h/adp78.c and Linux drivers/scsi/aic7xxx/
 * No public datasheet exists - Adaptec ASIC docs were NDA-only
 */

#include <array>
#include <o2emu/devices/device.h>
#include <o2emu/o2emu.h>
#include <vector>

namespace o2emu::devices {

class SCSIController : public Device {
public:
  SCSIController();
  ~SCSIController() override;

  // AIC-7880 register offsets (from PCI BAR0)
  enum Register : uint32_t {
    // Sequencer Control (0x0000 - 0x0010)
    REG_SEQCTL = 0x0000,
    REG_SEQADDR0 = 0x0004,
    REG_SEQADDR1 = 0x0008,
    REG_SEQADDR2 = 0x000C,
    REG_SEQADDR3 = 0x0010,
    REG_SEQCNT = 0x0014,
    REG_SEQFLAGS = 0x0018,
    REG_SEQCTL2 = 0x001C,

    // SCSI Control (0x0020 - 0x003C)
    REG_SCSISIG = 0x0020,
    REG_SCSIRATE = 0x0024,
    REG_SCSIID = 0x0028,
    REG_SCSILUN = 0x002C,
    REG_SCSISEQ = 0x0030,
    REG_SCSICNTL = 0x0034,
    REG_SCSISTAT = 0x0038,
    REG_SCSIFIFO = 0x003C,

    // Interrupt Control (0x0040 - 0x005C)
    REG_CLRINT = 0x0040,
    REG_INTSTAT = 0x0044,
    REG_SCSIINT = 0x0048,
    REG_SCSIINTEN = 0x004C,
    REG_SEQINT = 0x0050,
    REG_SEQINTEN = 0x0054,
    REG_BRKADDR = 0x0058,
    REG_BRKCTL = 0x005C,

    // DMA / Data Pointers (0x0060 - 0x008C)
    REG_DATAPTR0 = 0x0060,
    REG_DATAPTR1 = 0x0064,
    REG_DATAPTR2 = 0x0068,
    REG_DATAPTR3 = 0x006C,
    REG_DATACNT0 = 0x0070,
    REG_DATACNT1 = 0x0074,
    REG_DATACNT2 = 0x0078,
    REG_DATACNT3 = 0x007C,
    REG_HOSTADDR0 = 0x0080,
    REG_HOSTADDR1 = 0x0084,
    REG_HOSTADDR2 = 0x0088,
    REG_HOSTADDR3 = 0x008C,

    // SCB (SCSI Control Block) Registers (0x0090 - 0x00D0)
    REG_HCNT0 = 0x0090,
    REG_HCNT1 = 0x0094,
    REG_HCNT2 = 0x0098,
    REG_HCNT3 = 0x009C,
    REG_SCBPTR = 0x00A0,
    REG_SCBCNT = 0x00A4,
    REG_SCBCTL = 0x00A8,
    REG_SCBARRAY0 = 0x00AC,
    REG_SCBARRAY1 = 0x00B0,
    REG_SCBARRAY2 = 0x00B4,
    REG_SCBARRAY3 = 0x00B8,
    REG_SCB_TAG = 0x00BC,
    REG_SCB_LUN = 0x00C0,
    REG_SCB_CDBPTR0 = 0x00C4,
    REG_SCB_CDBPTR1 = 0x00C8,
    REG_SCB_CDBPTR2 = 0x00CC,
    REG_SCB_CDBPTR3 = 0x00D0,
    REG_SCB_CDBLEN = 0x00D4,
    REG_SCB_SGPTR0 = 0x00D8,
    REG_SCB_SGPTR1 = 0x00DC,
    REG_SCB_SGPTR2 = 0x00E0,
    REG_SCB_SGPTR3 = 0x00E4,
    REG_SCB_SGCNT = 0x00E8,
    REG_SCB_RESID0 = 0x00EC,
    REG_SCB_RESID1 = 0x00F0,
    REG_SCB_RESID2 = 0x00F4,
    REG_SCB_RESID3 = 0x00F8,
    REG_SCB_STATUS = 0x00FC,
    REG_SCB_SENSE = 0x0100,
    REG_SCB_MSG = 0x0104,
    REG_SCB_HFLAGS = 0x0108,

    // Host Configuration (0x010C - 0x011C)
    REG_HCONFIG = 0x010C,
    REG_HCONFIG2 = 0x0110,
    REG_HCONFIG3 = 0x0114,
    REG_HCONFIG4 = 0x0118,

    // Global Control/Status (0x011C - 0x0130)
    REG_GCTRL = 0x011C,
    REG_GSTAT = 0x0120,
    REG_BUSTIME = 0x0124,
    REG_BUSFREE = 0x0128,
    REG_SCSIOFF = 0x012C,
    REG_SCSION = 0x0130,
    REG_STIMEO = 0x0134,
    REG_SBLKCTL = 0x0138,

    // SCSI Rate/Offset/Timeout 2 (0x013C - 0x0148)
    REG_SCSIRATE2 = 0x013C,
    REG_SCSIOFF2 = 0x0140,
    REG_SCSION2 = 0x0144,
    REG_STIMEO2 = 0x0148,

    // SEEPROM (0x014C - 0x0158)
    REG_SEEPROM = 0x014C,
    REG_SEECTL = 0x0150,
    REG_SEEADDR = 0x0154,
    REG_SEEDATA = 0x0158,

    // Sequencer RAM Access (0x015C - 0x0168)
    REG_RAMPS = 0x015C,
    REG_RAMWS = 0x0160,
    REG_RAMRD = 0x0164,
    REG_RAMWD = 0x0168,

    // Scratch Registers (0x016C - 0x01AC)
    REG_SCRATCH0 = 0x016C,
    REG_SCRATCH1 = 0x0170,
    REG_SCRATCH2 = 0x0174,
    REG_SCRATCH3 = 0x0178,
    REG_SCRATCH4 = 0x017C,
    REG_SCRATCH5 = 0x0180,
    REG_SCRATCH6 = 0x0184,
    REG_SCRATCH7 = 0x0188,
    REG_SCRATCH8 = 0x018C,
    REG_SCRATCH9 = 0x0190,
    REG_SCRATCH10 = 0x0194,
    REG_SCRATCH11 = 0x0198,
    REG_SCRATCH12 = 0x019C,
    REG_SCRATCH13 = 0x01A0,
    REG_SCRATCH14 = 0x01A4,
    REG_SCRATCH15 = 0x01A8,

    // PCI Configuration (in PCI config space)
    PCI_VENDOR_ID = 0x00,
    PCI_DEVICE_ID = 0x02,
    PCI_COMMAND = 0x04,
    PCI_STATUS = 0x06,
    PCI_REVISION = 0x08,
    PCI_CLASS_CODE = 0x09,
    PCI_CACHE_LINE = 0x0C,
    PCI_LATENCY = 0x0D,
    PCI_HEADER_TYPE = 0x0E,
    PCI_BIST = 0x0F,
    PCI_BAR0 = 0x10, // I/O base
    PCI_BAR1 = 0x14, // Memory base
    PCI_SUBSYS_VENDOR = 0x2C,
    PCI_SUBSYS_ID = 0x2E,
    PCI_ROM_BASE = 0x30,
    PCI_INT_LINE = 0x3C,
    PCI_INT_PIN = 0x3D,
    PCI_MIN_GNT = 0x3E,
    PCI_MAX_LAT = 0x3F,
  };

  // HCNTRL bits
  enum HcntrlBit : uint32_t {
    HCNTRL_CHIP_RST = 0x80000000,
    HCNTRL_PCI_RST = 0x40000000,
    HCNTRL_LASTPHASE = 0x20000000,
    HCNTRL_INTEN = 0x10000000,
    HCNTRL_TERM_CTL = 0x08000000,
    HCNTRL_DPARCKEN = 0x04000000,
    HCNTRL_APARCKEN = 0x02000000,
    HCNTRL_SERR_EN = 0x01000000,
    HCNTRL_PERR_EN = 0x00800000,
    HCNTRL_MASTER_EN = 0x00400000,
    HCNTRL_MEM_EN = 0x00200000,
    HCNTRL_IO_EN = 0x00100000,
  };

  // SCSI Command codes
  enum ScsiCmd : uint8_t {
    SCSI_CMD_NOP = 0x00,
    SCSI_CMD_FLUSH_FIFO = 0x01,
    SCSI_CMD_RESET_ATN = 0x02,
    SCSI_CMD_RESET_DEV = 0x03,
    SCSI_CMD_RESET_BUS = 0x04,
    SCSI_CMD_XFER_INFO = 0x10,
    SCSI_CMD_XFER_PAD = 0x11,
    SCSI_CMD_SET_ATN = 0x1A,
    SCSI_CMD_CLR_ATN = 0x1B,
    SCSI_CMD_SEL_ATN = 0x40,
    SCSI_CMD_SEL_ATN_STOP = 0x41,
    SCSI_CMD_SEL = 0x42,
    SCSI_CMD_RESEL = 0x43,
    SCSI_CMD_WAIT_SEL = 0x44,
    SCSI_CMD_EN_SEL = 0x45,
    SCSI_CMD_DIS_SEL = 0x46,
  };

  // Device interface
  u32 read32(u32 offset) override;
  u16 read16(u32 offset) override;
  u8 read8(u32 offset) override;

  void write32(u32 offset, u32 value) override;
  void write16(u32 offset, u16 value) override;
  void write8(u32 offset, u8 value) override;

  void reset() override;
  void tick(u64 cycles) override;

  // Device interface (base class virtual methods)
  bool read(u32 offset, u32 size, u32 &value) override;
  bool write(u32 offset, u32 size, u32 value) override;
  u32 interrupt_status() const override;

  // PCI config space access
  u32 config_read(u32 reg, u32 size);
  void config_write(u32 reg, u32 size, u32 value);

  // SCSI device management
  void attach_device(int target_id, int lun, const std::string &image_path);
  void detach_device(int target_id, int lun);

  // Internal register access
  u32 read_reg(u32 offset);
  void write_reg(u32 offset, u32 value);

private:
  std::array<u32, 0x1000 / 4> regs_ = {}; // 4KB register space

  // PCI config space
  std::array<u32, 256 / 4> pci_config_ = {};

  // SCSI state
  struct ScsiDevice {
    bool present = false;
    std::string image_path;
    u64 capacity = 0;
    u32 block_size = 512;
    bool removable = false;
    bool media_locked = false;
  };
  std::array<std::array<ScsiDevice, 8>, 16> devices_; // 16 targets, 8 LUNs each

  // Command state
  u8 current_cmd_ = 0;
  u32 transfer_count_ = 0;
  u32 fifo_count_ = 0;
  std::vector<u8> data_fifo_;

  // Sequencer state (simplified)
  bool sequencer_running_ = false;
  u32 sequencer_pc_ = 0;

  // Sequencer RAM (4KB)
  std::vector<u8> script_ram_;

  // Internal helper methods
  void start_sequencer();
  void execute_scb();
  void handle_ram_access(u32 offset, u32 value);
};

} // namespace o2emu::devices