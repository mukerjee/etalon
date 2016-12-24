#ifndef _ze_mcp_defs_h
#define _ze_mcp_defs_h

/* structures and definitions to handle:
   - hard-handoff
   - accessing special-reg/eeprom/conf-space through MMIO
   - dump the beginning of sram somwhere on reboot
 */

#define ZE_HARD_SWITCH_OFFSET (0x140000)
#define ZE_HARD_SWITCH ((struct ze_hard_switch*)ZE_HARD_SWITCH_OFFSET)

struct ze_hard_switch {
  unsigned magic;
  unsigned size;
  unsigned crc32;
  char mcp[1];
};

#define ZE_HARD_SWITCH_MAGIC 0x6c6f6165

/* ZE_MMIO_* define regions within BAR2.  See firmware_ze/bar2.c */
#define ZE_MMIO_EEPROM 0x00000U
#define ZE_MMIO_EEPROM_SIZE 0x80000U

#define ZE_MMIO_SREG 0xffe00U
#define ZE_MMIO_SREG_SIZE 0x200U

#define ZE_MMIO_CFG 0x80000U
#define ZE_MMIO_CFG_SIZE 0x1000U

#define ZE_MMIO_PCIEBUF 0x88000U
#define ZE_MMIO_PCIEBUF_SIZE 0x8000U

#define ZE_MMIO_P0BUF 0x90000U
#define ZE_MMIO_P0BUF_SIZE 0x10000U

#define ZE_MMIO_RTC 0xffde0U
#define ZE_MMIO_RTC_SIZE 0x4U

#define ZE_MMIO_CPUC 0xffde8U
#define ZE_MMIO_CPUC_SIZE 0x4U

#define ZE_MMIO_STATE 0xffdf8U
#define ZE_MMIO_STATE_SIZE 0x4U

#define ZE_MMIO_IRQ 0xffdf0U
#define ZE_MMIO_IRQ_SIZE 0x4U

/* 32KB: 0xf0000-0xf7fff */
#define ZE_MMIO_MSIX_TABLE 0xf0000U
#define ZE_MMIO_MSIX_TABLE_SIZE 0x8000U

/* 256B: 0xf9000-0xf90ff */
#define ZE_MMIO_MSIX_PBA 0xf9000U
#define ZE_MMIO_MSIX_PBA_SIZE 0x100U

/* saving sram on reboot */
struct ze_dump {
  unsigned magic; /* identify action to take */
  unsigned size; /* how much sram start to save */
  unsigned addr; /* where to put it */
  unsigned pcie; /* if non-zero save the pcie sram buf */
};

/* location of the ze_dump structure in memory
   see compile assert in boot.c, assume sizeof(ze_dump)+sizeof(boot_state) <= 5120  */
#define ZE_DUMP_OFF(highmem_start) ((unsigned long)(highmem_start) - 5U*1024U)

/* where the jtag part of a dump is put */
#define ZE_DUMP_JTAG_LEN (32 * 1024)
#define ZE_DUMP_JTAG_OFF(highmem) ((unsigned long)(highmem) - (64 * 1024))

/* ze_dump->magic  commands to coordinate between host and mcp0 */
#define ZE_DUMP_REQ 0x73617665 /* "save" */
#define ZE_DUMP_ACK 0x646f6e65 /* "done" */
#define ZE_DUMP_REQ_NEXT 0x7361766e /* "savn": ignore this reboot and save on next one*/



#endif
