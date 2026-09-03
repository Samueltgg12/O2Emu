/**
 * @file scsicontroller.cpp
 * @brief SCSI controller (AIC-7880) implementation
 */

#include <cstring>
#include <o2emu/devices/scsicontroller.h>
#include <o2emu/logging/logger.h>

namespace o2emu::devices {

SCSIController::SCSIController()
    : Device("AIC-7880", 0xC0000000, 0x10000) // ISA I/O space
      ,
      script_ram_(nullptr) {
  reset();
}

SCSIController::~SCSIController() { delete[] script_ram_; }

void SCSIController::reset() {
  Device::reset();
  std::memset(regs_, 0, sizeof(regs_));

  // Allocate script RAM (4KB)
  if (!script_ram_) {
    script_ram_ = new u8[4096];
  }
  std::memset(script_ram_, 0, 4096);

  // AIC-7880 register defaults
  // SEQCTL: 0x00
  regs_[REG_SEQCTL] = 0x00;

  // SEQADDR0-3: 0x01-0x04
  for (int i = 0; i < 4; ++i) {
    regs_[REG_SEQADDR0 + i] = 0x00;
  }

  // SEQCNT: 0x05
  regs_[REG_SEQCNT] = 0x00;

  // SEQFLAGS: 0x06
  regs_[REG_SEQFLAGS] = 0x00;

  // SEQCTL2: 0x07
  regs_[REG_SEQCTL2] = 0x00;

  // SCSISIG: 0x08
  regs_[REG_SCSISIG] = 0x00;

  // SCSIRATE: 0x09
  regs_[REG_SCSIRATE] = 0x00;

  // SCSIID: 0x0A
  regs_[REG_SCSIID] = 0x07; // Default ID 7

  // SCSILUN: 0x0B
  regs_[REG_SCSILUN] = 0x00;

  // SCSISEQ: 0x0C
  regs_[REG_SCSISEQ] = 0x00;

  // SCSICNTL: 0x0D
  regs_[REG_SCSICNTL] = 0x00;

  // SCSISTAT: 0x0E
  regs_[REG_SCSISTAT] = 0x00;

  // SCSIFIFO: 0x0F
  regs_[REG_SCSIFIFO] = 0x00;

  // CLRINT: 0x10
  regs_[REG_CLRINT] = 0x00;

  // INTSTAT: 0x11
  regs_[REG_INTSTAT] = 0x00;

  // SCSIINT: 0x12
  regs_[REG_SCSIINT] = 0x00;

  // SCSIINTEN: 0x13
  regs_[REG_SCSIINTEN] = 0x00;

  // SEQINT: 0x14
  regs_[REG_SEQINT] = 0x00;

  // SEQINTEN: 0x15
  regs_[REG_SEQINTEN] = 0x00;

  // BRKADDR: 0x16
  regs_[REG_BRKADDR] = 0x00;

  // BRKCTL: 0x17
  regs_[REG_BRKCTL] = 0x00;

  // DATAPTR: 0x18-0x1B
  for (int i = 0; i < 4; ++i) {
    regs_[REG_DATAPTR0 + i] = 0x00;
  }

  // DATACNT: 0x1C-0x1E
  for (int i = 0; i < 3; ++i) {
    regs_[REG_DATACNT0 + i] = 0x00;
  }

  // HOSTADDR: 0x1F-0x22
  for (int i = 0; i < 4; ++i) {
    regs_[REG_HOSTADDR0 + i] = 0x00;
  }

  // HCNT: 0x23-0x25
  for (int i = 0; i < 3; ++i) {
    regs_[REG_HCNT0 + i] = 0x00;
  }

  // SCBPTR: 0x26
  regs_[REG_SCBPTR] = 0x00;

  // SCBCNT: 0x27
  regs_[REG_SCBCNT] = 0x00;

  // SCBCTL: 0x28
  regs_[REG_SCBCTL] = 0x00;

  // SCBARRAY: 0x29-0x2C
  for (int i = 0; i < 4; ++i) {
    regs_[REG_SCBARRAY0 + i] = 0x00;
  }

  // SCB_TAG: 0x2D
  regs_[REG_SCB_TAG] = 0x00;

  // SCB_LUN: 0x2E
  regs_[REG_SCB_LUN] = 0x00;

  // SCB_CDBPTR: 0x2F-0x32
  for (int i = 0; i < 4; ++i) {
    regs_[REG_SCB_CDBPTR0 + i] = 0x00;
  }

  // SCB_CDBLEN: 0x33
  regs_[REG_SCB_CDBLEN] = 0x00;

  // SCB_SGPTR: 0x34-0x37
  for (int i = 0; i < 4; ++i) {
    regs_[REG_SCB_SGPTR0 + i] = 0x00;
  }

  // SCB_SGCNT: 0x38
  regs_[REG_SCB_SGCNT] = 0x00;

  // SCB_RESID: 0x39-0x3B
  for (int i = 0; i < 3; ++i) {
    regs_[REG_SCB_RESID0 + i] = 0x00;
  }

  // SCB_STATUS: 0x3C
  regs_[REG_SCB_STATUS] = 0x00;

  // SCB_SENSE: 0x3D
  regs_[REG_SCB_SENSE] = 0x00;

  // SCB_MSG: 0x3E
  regs_[REG_SCB_MSG] = 0x00;

  // SCB_HFLAGS: 0x3F
  regs_[REG_SCB_HFLAGS] = 0x00;

  // Configuration registers
  // HCONFIG: 0x40
  regs_[REG_HCONFIG] = 0x00;

  // HCONFIG2: 0x41
  regs_[REG_HCONFIG2] = 0x00;

  // HCONFIG3: 0x42
  regs_[REG_HCONFIG3] = 0x00;

  // HCONFIG4: 0x43
  regs_[REG_HCONFIG4] = 0x00;

  // GCTRL: 0x44
  regs_[REG_GCTRL] = 0x00;

  // GSTAT: 0x45
  regs_[REG_GSTAT] = 0x00;

  // BUSTIME: 0x46
  regs_[REG_BUSTIME] = 0x00;

  // BUSFREE: 0x47
  regs_[REG_BUSFREE] = 0x00;

  // SCSIOFF: 0x48
  regs_[REG_SCSIOFF] = 0x00;

  // SCSION: 0x49
  regs_[REG_SCSION] = 0x00;

  // STIMEO: 0x4A
  regs_[REG_STIMEO] = 0x00;

  // SBLKCTL: 0x4B
  regs_[REG_SBLKCTL] = 0x00;

  // SCSIRATE2: 0x4C
  regs_[REG_SCSIRATE2] = 0x00;

  // SCSIOFF2: 0x4D
  regs_[REG_SCSIOFF2] = 0x00;

  // SCSION2: 0x4E
  regs_[REG_SCSION2] = 0x00;

  // STIMEO2: 0x4F
  regs_[REG_STIMEO2] = 0x00;

  // SEEPROM: 0x50
  regs_[REG_SEEPROM] = 0x00;

  // SEECTL: 0x51
  regs_[REG_SEECTL] = 0x00;

  // SEEADDR: 0x52
  regs_[REG_SEEADDR] = 0x00;

  // SEEDATA: 0x53
  regs_[REG_SEEDATA] = 0x00;

  // RAMPS: 0x54
  regs_[REG_RAMPS] = 0x00;

  // RAMWS: 0x55
  regs_[REG_RAMWS] = 0x00;

  // RAMRD: 0x56
  regs_[REG_RAMRD] = 0x00;

  // RAMWD: 0x57
  regs_[REG_RAMWD] = 0x00;

  // SCRATCH: 0x58-0x5F
  for (int i = 0; i < 8; ++i) {
    regs_[REG_SCRATCH0 + i] = 0x00;
  }
}

u32 SCSIController::read_reg(u32 offset) {
  if (offset < sizeof(regs_)) {
    return regs_[offset];
  }
  return 0;
}

void SCSIController::write_reg(u32 offset, u32 value) {
  if (offset >= sizeof(regs_))
    return;

  switch (offset) {
  case REG_SEQCTL:
    regs_[offset] = value;
    if (value & 0x80) { // Start sequencer
      start_sequencer();
    }
    break;

  case REG_CLRINT:
    // Writing clears interrupts
    regs_[REG_INTSTAT] &= ~value;
    regs_[REG_SEQINT] &= ~value;
    regs_[REG_SCSIINT] &= ~value;
    break;

  case REG_SCSIINTEN:
  case REG_SEQINTEN:
    regs_[offset] = value;
    break;

  case REG_SCBCTL:
    regs_[offset] = value;
    if (value & 0x01) { // SCB enable
      execute_scb();
    }
    break;

  case REG_GCTRL:
    regs_[offset] = value;
    if (value & 0x01) { // Chip reset
      reset();
    }
    break;

  case REG_SEECTL:
    // SEEPROM control
    regs_[offset] = value;
    break;

  case REG_SEEADDR:
    regs_[offset] = value;
    break;

  case REG_SEEDATA:
    regs_[offset] = value;
    break;

  case REG_RAMPS:
  case REG_RAMWS:
  case REG_RAMRD:
  case REG_RAMWD:
    // Script RAM access
    handle_ram_access(offset, value);
    break;

  default:
    regs_[offset] = value;
    break;
  }
}

void SCSIController::start_sequencer() {
  O2EMU_LOG_DEBUG("AIC-7880 sequencer started");
  // In a real implementation, this would execute SCSI scripts
  regs_[REG_SEQINT] |= 0x01; // Sequencer done
}

void SCSIController::execute_scb() {
  O2EMU_LOG_DEBUG("AIC-7880 SCB execution started");
  // In a real implementation, this would execute a SCSI command
  regs_[REG_SCB_STATUS] = 0x00; // Success
  regs_[REG_SCSIINT] |= 0x01;   // SCB complete
}

void SCSIController::handle_ram_access(u32 reg, u32 value) {
  u32 addr = regs_[REG_RAMPS] << 8 | regs_[REG_RAMWS];

  switch (reg) {
  case REG_RAMRD:
    regs_[REG_RAMRD] = script_ram_[addr];
    break;
  case REG_RAMWD:
    script_ram_[addr] = value;
    break;
  }
}

bool SCSIController::read(u32 offset, u32 size, u32 &value) {
  if (offset < sizeof(regs_)) {
    value = read_reg(offset);
    return true;
  }
  return false;
}

bool SCSIController::write(u32 offset, u32 size, u32 value) {
  if (offset < sizeof(regs_)) {
    write_reg(offset, value);
    return true;
  }
  return false;
}

void SCSIController::tick(u64 cycles) {
  // SCSI controller doesn't need periodic updates in this simple implementation
}

u32 SCSIController::interrupt_status() const { return regs_[REG_INTSTAT]; }

} // namespace o2emu::devices