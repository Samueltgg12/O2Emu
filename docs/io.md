# O2 (IP32) I/O

## MACE (I/O Engine ASIC)

MACE provides the bulk of the O2's I/O:

- **64-bit PCI bus** (single expansion slot)
- **ISA bus** (used solely for the Super I/O chip)
- **2× PS/2 ports** (keyboard + mouse)
- **10/100 Base-T Ethernet**
- Audio (MACEISA audio)

### MACE registers

- `MACE_BASE = 0x1f000000` (physical)
- See [register-maps.md](register-maps.md) for the full register map

## PCI bus

- 64-bit, provided by MACE
- Single expansion slot
- PCI devices (from `fixup-ip32.c`):
  - aic7xxx SCSI controller (2 devices: SCSI0, SCSI1)
  - Expansion slot
  - N/C, N/C
- IRQ routing: SCSI0/SCSI1/SLOT0-2/SHARED0-2

## SCSI

- UltraWide SCSI
- Adaptec AIC-7880 controller
- R5000/RM7000 units: 2 drive sleds
- R10000/R12000 units: 1 drive sled

## ISA bus

- Used solely for the Super I/O chip
- Provides serial + parallel ports

## Ethernet

- 10/100 Base-T
- Provided by MACE

## Audio

- MACEISA audio (see register-maps.md for MACEISA_AUDIO_SW_IRQ)

## NetBSD driver

- `sys/arch/sgimips/dev/mace.c` — MACE driver
- `sys/arch/sgimips/dev/hpcreg.h` — HPC (?) register definitions

## TODO / Open questions

- [ ] Full MACE register map
- [ ] MACEISA audio register details (mavb)
- [ ] PCI configuration space details
- [ ] Ethernet controller register details