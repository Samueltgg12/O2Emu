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

MACE sub-blocks (from the decompiled PROM `definitions.h`):

| Offset | Device |
|--------|--------|
| `0x080000` | PCI host bridge |
| `0x280000` | Ethernet |
| `0x300000` | Peripheral (Audio, ISA, KBD/MS, I2C, UST) |
| `0x380000` | ISA external (UART1, UART2, RTC) |

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

## UART (serial)

- Two 16550-style UARTs at `0x1f390000` (UART1) and `0x1f398000` (UART2)
- Registers byte-addressed: `UART_REG(x) = (x << 8) + 7`
- See [register-maps.md](register-maps.md) for the register list

## RTC

- MC146818-style RTC at `0x1f3a0000`
- Registers byte-addressed: `RTC_REG(x) = x << 8`
- Includes NVRAM (`RTC_NVRAM(x) = (0x0e + x) << 8`)
- See [register-maps.md](register-maps.md) for the register list

## PS/2

- Keyboard + mouse ports at `0x1f320000` (MACE peripheral KBD/MS)
- TX/RX buffers, control, and status registers for each
- See [register-maps.md](register-maps.md) for the register list

## I2C

- I2C controller at `0x1f330000` (MACE peripheral I2C)
- Config, status, and data registers
- Used to read monitor EDID
- See [register-maps.md](register-maps.md) for the register list

## NetBSD driver

- `sys/arch/sgimips/dev/mace.c` — MACE driver
- `sys/arch/sgimips/dev/hpcreg.h` — HPC (?) register definitions

## TODO / Open questions

- [ ] MACEISA audio register details (mavb)
- [ ] PCI configuration space details
- [ ] Ethernet controller register details (beyond the decompiled map)