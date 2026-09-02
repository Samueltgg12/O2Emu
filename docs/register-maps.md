# O2 (IP32) Register Maps — from Linux/NetBSD drivers

Register-level details mined from open-source drivers. These are the ground
truth for emulator implementation.

## Memory Map (Linux `arch/mips/include/asm/ip32/mace.h`)

- `MACE_BASE = 0x1f000000` (physical)

## CRIME ASIC (Linux `arch/mips/include/asm/ip32/crime.h`)

CRIME is the memory controller / system controller ASIC.

### CRIME Control register bits
```
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

## TODO / Open questions

- [ ] Full CRIME register map (memory controller registers, ECC)
- [ ] Full MACE register map (Ethernet, Super I/O, audio, PCI bridge)
- [ ] ICE ASIC register map (no NetBSD driver; needs Linux/IRIX source)
- [ ] MRE (Memory & Rendering Engine) register map
- [ ] Display Engine register map
- [ ] crmfb framebuffer register map
- [ ] mavb (Moosehead audio) register map