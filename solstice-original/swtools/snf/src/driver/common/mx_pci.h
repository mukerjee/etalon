/*************************************************************************
 * The contents of this file are subject to the MYRICOM MYRINET          *
 * EXPRESS (MX) NETWORKING SOFTWARE AND DOCUMENTATION LICENSE (the       *
 * "License"); User may not use this file except in compliance with the  *
 * License.  The full text of the License can found in LICENSE.TXT       *
 *                                                                       *
 * Software distributed under the License is distributed on an "AS IS"   *
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See  *
 * the License for the specific language governing rights and            *
 * limitations under the License.                                        *
 *                                                                       *
 * Copyright 2003 - 2004 by Myricom, Inc.  All rights reserved.          *
 *************************************************************************/

#ifndef MX_PCI_H
#define MX_PCI_H

typedef struct mx_myrinet_eeprom
{
  uint32_t lanai_clockval;        /* 00-03 */ 
  uint16_t lanai_cpu_version;     /* 04-05 */ 
  uint8_t  lanai_board_id[6];     /* 06-0B */ 
  uint32_t lanai_sram_size;       /* 0C-0F */ 
  uint8_t  fpga_version[32];      /* 10-2F */ 
  uint8_t  more_version[16];      /* 30-3F */ 
                                                    
  uint16_t delay_line_value;      /* 40-41 */ 
  uint16_t board_type;            /* 42-43 */ 
  uint16_t bus_type;              /* 44-45 */ 
  uint16_t product_code;          /* 46-47 */ 
  uint32_t serial_number;         /* 48-4B */ 
  uint8_t  board_label[32];       /* 4C-6B */ 
  uint16_t max_lanai_speed;       /* 6C-6D */ 
  uint16_t future_use[7];         /* 6E-7B */ 
  uint32_t unused_4_bytes;        /* 7C-7F */ 
} mx_myrinet_eeprom_t;

/********************
 * PCI configuration registers definitions (as described in the PCI 2.1 spec)
 ********************/

typedef struct mx_pci_config
{
  uint16_t Vendor_ID;
  uint16_t Device_ID;
  uint16_t Command;
  uint16_t Status;
  uint8_t Revision_ID;
  uint8_t Class_Code_Programming_Interface;
  uint8_t Class_Code_Subclass;
  uint8_t Class_Code_Base_Class;
  uint8_t Cache_Line_Size;
  uint8_t Latency_Timer;
  uint8_t Header_Type;
  uint8_t bist;
  uint32_t Base_Addresses_Registers[6];
  uint32_t Cardbus_CIS_Pointer;
  uint16_t Subsystem_Vendor_ID;
  uint16_t Subsystem_ID;
  uint32_t Expansion_ROM_Base_Address;
  uint8_t Cap_List; /* bottom 2 bits reserved. */
  uint8_t Reserved[7];
  uint8_t Interrupt_Line;
  uint8_t Interrupt_Pin;
  uint8_t Min_Gnt;
  uint8_t Max_Lat;
  uint8_t device_specific[192];
} mx_pci_config_t;

#define  MX_PCI_COMMAND		    4
#define  MX_PCI_COMMAND_IO          0x1 /* Enable response in I/O space */
#define  MX_PCI_COMMAND_MEMORY      0x2 /* Enable response in Memory space */
#define  MX_PCI_COMMAND_MASTER      0x4 /* Enable bus mastering */
#define  MX_PCI_COMMAND_SPECIAL     0x8 /* Enable response to special cycles */
#define  MX_PCI_COMMAND_INVALIDATE  0x10        /* Use memory write and invalida
te */
#define  MX_PCI_COMMAND_VGA_PALETTE 0x20        /* Enable palette snooping */
#define  MX_PCI_COMMAND_PARITY      0x40        /* Enable parity checking */
#define  MX_PCI_COMMAND_WAIT        0x80        /* Enable address/data stepping
*/
#define  MX_PCI_COMMAND_SERR        0x100       /* Enable SERR */
#define  MX_PCI_COMMAND_FAST_BACK   0x200       /* Enable back-to-back writes */
#define  MX_PCI_COMMAND_INTX_DISABLE 0x400       /* Disable legacy int */

#define  MX_PCI_STATUS              6
#define  MX_PCI_STATUS_MABORT       0x2000      /* Dev detected master abort */
#define  MX_PCI_STATUS_PERR         0x8000      /* Dev detected parity err  */

/* Extract the Capabilities_List bit from the PCI config Status field value. */
#define  MX_PCI_STATUS_CAPABILITIES_LIST(x) ((x>>4)&1)

#define MX_PCI_VENDOR_MYRICOM   0x14c1
#define MX_PCI_DEVICE_MYRINET   0x8043
#define MX_PCI_DEVICE_Z4E   	0x0007
#define MX_PCI_DEVICE_Z8E   	0x0008
#define MX_PCI_DEVICE_Z8E_9   	0x0009

#define MX_Z8_PCIE_DEVCTL 0x64
#define MX_PCIE_DEVCTL_READRQ 0x7000

#define MX_PCI_BASE_ADDRESS_TYPE(val) (((val) >> 1) & 3)

/* PCI config space capabilities structure.  These are chained with
   the Next_Pointer field, which is, ignoring the bottom 2 bits, the
   offset to the next capability in the PCI config space. */

typedef union mx_pci_capability
{
  struct
  {
    uint8_t Capability_ID;
    uint8_t Next_Pointer;	/* bottom 2 bits are reserved. */
  } common;
  
  /* Message signalled interrupt capability with 64-bit address. */
  struct 
  {
    uint8_t Capability_ID;
    uint8_t Next_Pointer;	/* bottom 2 bits are reserved. */
    uint16_t Message_Control;
    /* ... */
  } msi;

  /* Message signalled interrupt capability with 64-bit address. */
  struct 
  {
    uint8_t Capability_ID;
    uint8_t Next_Pointer;	/* bottom 2 bits are reserved. */
    uint16_t Message_Control;
    uint32_t Message_Address;
    uint32_t Message_Upper_Address;
    uint16_t Message_Data;
  } msi64;

  /* PCI-X capability */
  struct
  {
    uint8_t Capability_ID;
    uint8_t Next_Pointer;
    uint16_t Command;
    uint16_t Status;
  } pci_x;
  
} mx_pci_capability_t;

/* Strip the reserved bits from the bottom of a capability pointer. */
   
#define MX_PCI_CAP_POINTER_CLEANUP(x) ((x)&0xfc)

/* Extract the bit fields from a PCI capability's "Message_Control" field. */

#define MX_PCI_CAP_MESSAGE_CONTROL_64_BIT_ADDRESS_CAPABLE(x) (((x)>>7)&1)
#define MX_PCI_CAP_MESSAGE_CONTROL_MULTIPLE_MESSAGE_ENABLE(x) (((x)>>4)&0x7)
#define MX_PCI_CAP_MESSAGE_CONTROL_MULTIPLE_MESSAGE_CAPABLE(x) (((x)>>1)&0x7)
#define MX_PCI_CAP_MESSAGE_CONTROL_MSI_ENABLE(x) (((x)>>0)&1)
#define MX_PCI_CAP_PCI_X_COMMAND_MAX_MEM_READ_BYTE_CNT_MASK (0xC)
#define MX_PCI_CAP_PCI_X_COMMAND_MAX_MEM_READ_BYTE_CNT_4096 (0xC)
#define MX_PCI_CAP_PCI_X_COMMAND_MAX_MEM_READ_BYTE_CNT_2048 (0x8)
#define MX_PCI_CAP_PCI_X_COMMAND_MAX_MEM_READ_BYTE_CNT_1024 (0x4)
#define MX_PCI_CAP_PCI_X_COMMAND_MAX_MEM_READ_BYTE_CNT_512  (0x0)

/* Do we want to force PCI-X to use large read blocks?  By default
   we do this, but it is conceivable that some very small fraction of
   customers will not want to do this if other cards in their system
   have real-time constraints that cannot be met with large block
   sizes.  If we don't force large reads, receive DMA performance has
   been observed to drop by up to 50%! */
#define MX_PCI_X_FORCE_LARGE_READ 1

#define MX_PCI_EXP_LNK_STA 0x12
#define MX_PCI_EXP_LNK_WIDTH(lnk_sta) ((lnk_sta & 0x3f0) >> 4)

enum mx_pci_capability_id
  {
    MX_PCI_CAP_RESERVED = 0,
    MX_PCI_CAP_PCI_POWER_MANAGEMENT_INTERFACE = 1,
    MX_PCI_CAP_AGP = 2,
    MX_PCI_CAP_VPD = 3,
    MX_PCI_CAP_SLOT_IDENTIFICATION = 4,
    MX_PCI_CAP_MESSAGE_SIGNALLED_INTERRUPTS = 5,
    MX_PCI_CAP_COMPACTPCI_HOT_SWAP = 6,
    MX_PCI_CAP_PCI_X = 7,
    MX_PCI_CAP_VENDOR = 9,
    MX_PCI_CAP_EXP = 0x10
  };

/************************************
 ** LANai Memory-mapped registers **
 ************************************/

struct LANaiX_readable_specials {
  /* 0xfffffc00 */ uint32_t P0_RPL_CONFIG;
						uint8_t pad4[0x4];
  /* 0xfffffc08 */ uint32_t P0_RB;
                                                uint8_t padc[0x4];
  /* 0xfffffc10 */ uint16_t P0_RPL_INDEX;
  /* 0xfffffc12 */ uint16_t P0_RECV_COUNT;
						uint8_t pad14[0x4];
  /* 0xfffffc18 */ uint32_t P0_A_RPL_CONFIG;
						uint8_t pad1c[0x4];
  /* 0xfffffc20 */ uint32_t P0_A_RB;
                                                uint8_t pad24[0x4];
  /* 0xfffffc28 */ uint16_t P0_A_RPL_INDEX;
  /* 0xfffffc2a */ uint16_t P0_A_RECV_COUNT;
						uint8_t pad2c[0x6];
  /* 0xfffffc32 */ uint16_t P0_BUFFER_DROP;
						uint8_t pad34[0x6];
  /* 0xfffffc3a */ uint16_t P0_A_BUFFER_DROP;
						uint8_t pad3c[0x4];
  /* 0xfffffc40 */ uint16_t P0_MEMORY_DROP;
						uint8_t pad42[0x9];
  /* 0xfffffc4b */ uint8_t P0_SEND_FREE_COUNT;
						uint8_t pad4c[0x7];
  /* 0xfffffc53 */ uint8_t P0_SEND_FREE_LIMIT;
						uint8_t pad54[0x7];
  /* 0xfffffc5b */ uint8_t P0_SEND_COUNT;
						uint8_t pad5c[0x7];
  /* 0xfffffc63 */ uint8_t P0_PAUSE_COUNT;
						uint8_t pad64[0x7];
  /* 0xfffffc6b */ uint8_t P0_SEND_CONTROL;
						uint8_t pad6c[0x4];
  /* 0xfffffc70 */ uint16_t PORT_ADDR;
						uint8_t pad72[0x6];
  /* 0xfffffc78 */ uint16_t PORT_DATA;
						uint8_t pad7a[0x6];
  /* 0xfffffc80 */ uint32_t P1_RPL_CONFIG;
						uint8_t pad84[0x4];
  /* 0xfffffc88 */ uint32_t P1_RB;
                                                uint8_t pad8c[0x4];
  /* 0xfffffc90 */ uint16_t P1_RPL_INDEX;
  /* 0xfffffc92 */ uint16_t P1_RECV_COUNT;
						uint8_t pad94[0x4];
  /* 0xfffffc98 */ uint32_t P1_A_RPL_CONFIG;
						uint8_t pad9c[0x4];
  /* 0xfffffca0 */ uint32_t P1_A_RB;
                                                uint8_t pada4[0x4];
  /* 0xfffffca8 */ uint16_t P1_A_RPL_INDEX;
  /* 0xfffffcaa */ uint16_t P1_A_RECV_COUNT;
						uint8_t padac[0x6];
  /* 0xfffffcb2 */ uint16_t P1_BUFFER_DROP;
						uint8_t padb4[0x6];
  /* 0xfffffcba */ uint16_t P1_A_BUFFER_DROP;
						uint8_t padbc[0x4];
  /* 0xfffffcc0 */ uint16_t P1_MEMORY_DROP;
						uint8_t padc2[0x9];
  /* 0xfffffccb */ uint8_t P1_SEND_FREE_COUNT;
						uint8_t padcc[0x7];
  /* 0xfffffcd3 */ uint8_t P1_SEND_FREE_LIMIT;
						uint8_t padd4[0x7];
  /* 0xfffffcdb */ uint8_t P1_SEND_COUNT;
						uint8_t paddc[0x7];
  /* 0xfffffce3 */ uint8_t P1_PAUSE_COUNT;
						uint8_t pade4[0x7];
  /* 0xfffffceb */ uint8_t P1_SEND_CONTROL;
						uint8_t padec[0x16];
  /* 0xfffffd02 */ uint8_t CRC32_CONFIG;
						uint8_t pad103[0x5];
  /* 0xfffffd08 */ uint32_t CRC32;
						uint8_t pad10c[0x1c];
  /* 0xfffffd28 */ uint32_t CPUC;
						uint8_t pad12c[0x4];
  /* 0xfffffd30 */ uint32_t RTC;
						uint8_t pad134[0x4];
  /* 0xfffffd38 */ uint32_t IT0;
						uint8_t pad13c[0x4];
  /* 0xfffffd40 */ uint32_t IT1;
						uint8_t pad144[0x4];
  /* 0xfffffd48 */ uint32_t IT2;
						uint8_t pad14c[0x4];
  /* 0xfffffd50 */ uint8_t LED;
						uint8_t pad151[0x7];
  /* 0xfffffd58 */ uint8_t MDI;
						uint8_t pad159[0x7];
  /* 0xfffffd60 */ uint32_t SYNC;
						uint8_t pad164[0x4];
  /* 0xfffffd68 */ uint32_t PLL;
						uint8_t pad16c[0x4];
  /* 0xfffffd70 */ uint16_t DISPATCH_INDEX;
						uint8_t pad172[0x2e];
  /* 0xfffffda0 */ uint8_t ARBITER_SLOT;
						uint8_t pad1a1[0x7];
  /* 0xfffffda8 */ uint8_t ARBITER_CODE;
						uint8_t pad1a9[0x7];
  /* 0xfffffdb0 */ uint32_t MP_LOWER;
						uint8_t pad1b4[0x4];
  /* 0xfffffdb8 */ uint32_t MP_UPPER;
						uint8_t pad1bc[0x4];
  /* 0xfffffdc0 */ uint32_t JTAG_MASTER;
						uint8_t pad1c4[0x1c];
  /* 0xfffffde0 */ uint32_t AISR_OFF;
						uint8_t pad1e4[0x4];
  /* 0xfffffde8 */ uint32_t AISR_ON;
						uint8_t pad1ec[0x4];
  /* 0xfffffdf0 */ uint32_t AISR;
						uint8_t pad1f4[0x4];
  /* 0xfffffdf8 */ uint32_t ISR;
						uint8_t pad1fc[0x84];
  /* 0xfffffe80 */ uint32_t DMA_CONFIG;
						uint8_t pad284[0x6];
  /* 0xfffffe8a */ uint16_t PCI_CLOCK;
						uint8_t pad28c[0x4];
  /* 0xfffffe90 */ uint8_t DMA0_COUNT;
						uint8_t pad291[0x8];
  /* 0xfffffe99 */ uint8_t DMA1_COUNT;
						uint8_t pad29a[0x8];
  /* 0xfffffea2 */ uint8_t DMA2_COUNT;
						uint8_t pad2a3[0x8];
  /* 0xfffffeab */ uint8_t DMA3_COUNT;
						uint8_t pad2ac[0x4];
  /* 0xfffffeb0 */ uint64_t DMA0_POINTER;
  /* 0xfffffeb8 */ uint64_t DMA1_POINTER;
  /* 0xfffffec0 */ uint64_t DMA2_POINTER;
  /* 0xfffffec8 */ uint64_t DMA3_POINTER;
};
struct LANaiX_writable_specials {
  /* 0xfffffc00 */ uint32_t P0_RPL_CONFIG;
						uint8_t pad4[0x4];
  /* 0xfffffc08 */ uint32_t P0_RB;
                                                uint8_t padc[0x4];
						uint8_t pad10[0x8];
  /* 0xfffffc18 */ uint32_t P0_A_RPL_CONFIG;
						uint8_t pad1c[0x4];
  /* 0xfffffc20 */ uint32_t P0_A_RB;
                                                uint8_t pad24[0x4];
						uint8_t pad28[0x20];
  /* 0xfffffc48 */ uint64_t P0_SEND;
						uint8_t pad50[0x3];
  /* 0xfffffc53 */ uint8_t P0_SEND_FREE_LIMIT;
						uint8_t pad54[0x7];
  /* 0xfffffc5b */ uint8_t P0_SEND_COUNT;
						uint8_t pad5c[0x7];
  /* 0xfffffc63 */ uint8_t P0_PAUSE_COUNT;
						uint8_t pad64[0x7];
  /* 0xfffffc6b */ uint8_t P0_SEND_CONTROL;
						uint8_t pad6c[0x4];
  /* 0xfffffc70 */ uint16_t PORT_ADDR;
						uint8_t pad72[0x6];
  /* 0xfffffc78 */ uint16_t PORT_DATA;
						uint8_t pad7a[0x6];
  /* 0xfffffc80 */ uint32_t P1_RPL_CONFIG;
						uint8_t pad84[0x4];
  /* 0xfffffc88 */ uint32_t P1_RB;
                                                uint8_t pad8c[0x4];
						uint8_t pad90[0x8];
  /* 0xfffffc98 */ uint32_t P1_A_RPL_CONFIG;
						uint8_t pad9c[0x4];
  /* 0xfffffca0 */ uint32_t P1_A_RB;
                                                uint8_t pada4[0x4];
						uint8_t pada8[0x18];
  /* 0xfffffcc0 */ uint16_t P1_MEMORY_DROP;
						uint8_t padc2[0x6];
  /* 0xfffffcc8 */ uint64_t P1_SEND;
						uint8_t padd0[0x3];
  /* 0xfffffcd3 */ uint8_t P1_SEND_FREE_LIMIT;
						uint8_t padd4[0x7];
  /* 0xfffffcdb */ uint8_t P1_SEND_COUNT;
						uint8_t paddc[0x7];
  /* 0xfffffce3 */ uint8_t P1_PAUSE_COUNT;
						uint8_t pade4[0x7];
  /* 0xfffffceb */ uint8_t P1_SEND_CONTROL;
						uint8_t padec[0x4];
  /* 0xfffffcf0 */ uint32_t COPY;
						uint8_t padf4[0x6];
  /* 0xfffffcfa */ uint8_t COPY_ABORT;
						uint8_t padfb[0x7];
  /* 0xfffffd02 */ uint8_t CRC32_CONFIG;
						uint8_t pad103[0x5];
  /* 0xfffffd08 */ uint32_t CRC32;
						uint8_t pad10c[0x6];
  /* 0xfffffd12 */ uint8_t CRC32_BYTE;
						uint8_t pad113[0x5];
  /* 0xfffffd18 */ uint16_t CRC32_HALF;
						uint8_t pad11a[0x6];
  /* 0xfffffd20 */ uint32_t CRC32_WORD;
						uint8_t pad124[0x4];
  /* 0xfffffd28 */ uint32_t CPUC;
						uint8_t pad12c[0x4];
 /* 0xfffffd30 */ uint32_t RTC;
						uint8_t pad134[0x4];
  /* 0xfffffd38 */ uint32_t IT0;
						uint8_t pad13c[0x4];
  /* 0xfffffd40 */ uint32_t IT1;
						uint8_t pad144[0x4];
  /* 0xfffffd48 */ uint32_t IT2;
						uint8_t pad14c[0x4];
  /* 0xfffffd50 */ uint8_t LED;
						uint8_t pad151[0x7];
  /* 0xfffffd58 */ uint8_t MDI;
						uint8_t pad159[0x7];
  /* 0xfffffd60 */ uint32_t SYNC;
						uint8_t pad164[0x4];
  /* 0xfffffd68 */ uint32_t PLL;
						uint8_t pad16c[0x4];
  /* 0xfffffd70 */ uint32_t DISPATCH_STATE;
						uint8_t pad174[0x4];
  /* 0xfffffd78 */ uint32_t DISPATCH_ISR_ON;
						uint8_t pad17c[0x4];
  /* 0xfffffd80 */ uint32_t DISPATCH_ISR_OFF;
						uint8_t pad184[0x4];
  /* 0xfffffd88 */ uint32_t DISPATCH_STATE_ON;
						uint8_t pad18c[0x4];
  /* 0xfffffd90 */ uint32_t DISPATCH_STATE_OFF;
						uint8_t pad194[0x4];
  /* 0xfffffd98 */ uint32_t DISPATCH_CONFIG;
						uint8_t pad19c[0x4];
  /* 0xfffffda0 */ uint8_t ARBITER_SLOT;
						uint8_t pad1a1[0x7];
  /* 0xfffffda8 */ uint8_t ARBITER_CODE;
						uint8_t pad1a9[0x7];
  /* 0xfffffdb0 */ uint32_t MP_LOWER;
						uint8_t pad1b4[0x4];
  /* 0xfffffdb8 */ uint32_t MP_UPPER;
						uint8_t pad1bc[0x4];
  /* 0xfffffdc0 */ uint32_t JTAG_MASTER;
						uint8_t pad1c4[0x1c];
  /* 0xfffffde0 */ uint32_t AISR_OFF;
						uint8_t pad1e4[0x4];
  /* 0xfffffde8 */ uint32_t AISR_ON;
						uint8_t pad1ec[0x4];
  /* 0xfffffdf0 */ uint32_t AISR;
						uint8_t pad1f4[0x4];
  /* 0xfffffdf8 */ uint32_t ISR;
						uint8_t pad1fc[0x84];
  /* 0xfffffe80 */ uint32_t DMA_CONFIG;
						uint8_t pad284[0x6];
  /* 0xfffffe8a */ uint16_t PCI_CLOCK;
						uint8_t pad28c[0x4];
  /* 0xfffffe90 */ uint8_t DMA0_COUNT;
						uint8_t pad291[0x8];
  /* 0xfffffe99 */ uint8_t DMA1_COUNT;
						uint8_t pad29a[0x8];
  /* 0xfffffea2 */ uint8_t DMA2_COUNT;
						uint8_t pad2a3[0x8];
  /* 0xfffffeab */ uint8_t DMA3_COUNT;
						uint8_t pad2ac[0x4];
  /* 0xfffffeb0 */ uint64_t DMA0_POINTER;
  /* 0xfffffeb8 */ uint64_t DMA1_POINTER;
  /* 0xfffffec0 */ uint64_t DMA2_POINTER;
  /* 0xfffffec8 */ uint64_t DMA3_POINTER;
};

struct LANaiZ_specials {
  /* layout of BAR 2 */
  uint8_t eeprom[512*1024];
  union {
    mx_pci_config_t space;
    uint8_t space8[4096];
    uint16_t space16[2048];
    uint32_t space32[1024];
  } config;
  uint8_t pad[512*1024 - 4096 - 512];
  union {
    uint8_t w8[512];
    uint16_t w16[256];
    uint32_t w32[128];
  } sreg;
};

#define MX_LZ_ISR sreg.w32[0x1f8/4]

typedef union mx_lanai_special_registers
{
  union
  {
    struct LANaiX_readable_specials lx;
    struct LANaiZ_specials lz;
  } read;
  
  union
  {
    struct LANaiX_writable_specials lx;
    struct LANaiZ_specials lz;
  } write;
} mx_lanai_special_registers_t;

#define MX_LX_REQ_ACK_0		0x10000000 /* REQ_ACK_0 */
#define MX_LX_REQ_ACK_1		0x20000000 /* REQ_ACK_1 */
#define MX_LX_PARITY_INT	0x00100000 /* PARITY_INT */

#define MX_LX_SPECIAL_LEN	(1U*1024)
#define MX_LX_SPECIAL_OFFSET	(16U*1024*1024 - MX_LX_SPECIAL_LEN)
#define MX_LX_CONTROL_OFFSET    (8U*1024*1024)
#define MX_LX_CONTROL_LEN       (256)
#define	MX_LX_PCI_OFFSET	(0x0FFFFC00)
#define MX_EEPROM_STRINGS_LEN 256

/* The control register is little-endian,
   but using the usual pci conf space functions will
   make the endianess issue transparent for our use */

#define MX_LX_CTRL_CPU_RESET_ON_BIT       31
#define MX_LX_CTRL_CPU_RESET_OFF_BIT      30
#define MX_LX_CTRL_IO_RESET_ON_BIT        29
#define MX_LX_CTRL_IO_RESET_OFF_BIT       28
#define MX_LX_CTRL_PCI_INT_ENABLE_ON_BIT  27
#define MX_LX_CTRL_PCI_INT_ENABLE_OFF_BIT 26

#define MX_LZ_HANDOFF_LOC 0xfc0000U

struct mx_lz_handoff {
  unsigned bus_high;
  unsigned bus_low;
  unsigned data;
  unsigned sram_addr;
  unsigned mcp_len;
  unsigned to;
  unsigned jump_addr;
  unsigned word7;
  uint64_t pad[4];
};


#endif /* MX_PCI_H */

