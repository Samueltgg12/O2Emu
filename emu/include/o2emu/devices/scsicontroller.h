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
    // Sequencer Control
    SEQCTL = 0x0000,
    SEQADDR0 = 0x0004,
    SEQADDR1 = 0x0008,
    SEQADDR2 = 0x000C,
    SEQADDR3 = 0x0010,

    // Host Control
    HCNTRL = 0x0014,
    HCNTRL_PCI = 0x0018,

    // Interrupt
    INTSTAT = 0x001C,
    INTEN = 0x0020,

    // SCSI Control
    SCSICMD = 0x0024,
    SCSISEQ = 0x0028,
    SCSISIG = 0x002C,
    SCSIRATE = 0x0030,
    SCSIID = 0x0034,
    SCSILUN = 0x0038,

    // Data FIFO
    DFIFO = 0x003C,
    DFDAT = 0x0040,

    // Transfer Count
    TCH = 0x0044,
    TCM = 0x0048,
    TCL = 0x004C,

    // SCSI FIFO
    SFIFO = 0x0050,
    SFDAT = 0x0054,

    // SCSI Status
    SCSISTAT = 0x0058,
    SCSISTAT1 = 0x005C,

    // Queue Control
    QCTRL = 0x0060,
    QADDR = 0x0064,

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

  // PCI config space access
  u32 config_read(u32 reg, u32 size);
  void config_write(u32 reg, u32 size, u32 value);

  // SCSI device management
  void attach_device(int target_id, int lun, const std::string &image_path);
  void detach_device(int target_id, int lun);

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
};

} // namespace o2emu::devices