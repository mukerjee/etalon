/* EEPROM images are byte codes interpretted by the hardware starting
   at offset 0, and are layed out as follows:
   
   +---+----+------+---+--+------------+-----#------------+----
   | H | HW | MCP0 | T |SS| L1 MCP1 C1 | ... # L2 MCP2 C2 | ...
   +---+----+------+---+--+------------+-----#------------+----
   
   H = skipped ze_eeprom_header (see below)
   HW = CLOCK and scan path initialization
   SS = raw strings specs.
   MCP0 = loads MCP0 int SRAM, typically a bootstrap loader
   	  to uncompress and install MCP2||MCP1, but it may be a full
	  MCP for development purposes.
   T = takes NIC out of reset and terminates jtag interpretation.
   L1 = 32-bit length of MCP1 (high bit set if MCP1 is compressed)
   MCP1 = firmware bundle 1 (see below)
   C1 = crc-32 over MCP1.
   # = eeprom segment boundary
   "L2 MCP2 C2" are like "L1 MCP1 C1" except they specify the details
     of the optional reprogrammable MCP.  For development purposes both
     of these regions may be ommitted, but the ze_eeprom_header will
     always point to where they would be if present.

   MCP1 and MCP2 are firmware bundles of the form
       MCP_SRAM_IMAGE [PCI_ROM_LEN PCI_ROM_IMAGE] [BLOB_LEN BLOB]* MCP_SRAM_LEN
   , where each LEN is 32 bits, and the IMAGEs are any number of
   bytes.  ( MCP_SRAM_LEN is at the end of the firmware bundle for
   backwards with earlier software where MCP_SRAM_IMAGE was the only
   thing stored in the firmware bundle.) */

/* Layout of the start of an EEPROM image. */

struct eeprom_header_ze
{
  /* Reserved space for the JTAG byte code sequence to arrange to
     not interpret this header. */

  unsigned char __jump[8];

  /* EEPROM details, in case these change. */

  unsigned int eeprom_len;
  unsigned int eeprom_segment_len;

  /* The location of the defult (not-reprogrammable) MCP. */
  
  unsigned int mcp1_offset;

  /* The location of the optional reprogrammable MCP.  This offset is
     eeprom_segment_len-aligned.  All memory from mcp2_offset to
     eeprom_len is available for reprogramming. */
  
  unsigned int mcp2_offset;

  /* Space for future expansion. */

#define EEPROM_HEADER_ZE_VERSION 1
  unsigned int version;
  
#if EEPROM_HEADER_ZE_VERSION >= 1

  /* The location of a raw copy of the string specs in EEPROM.  This
     is generally not used, since another copy is in SRAM at address
     STRING_SPECS as defined in lanai_*_def.h, but it might come in
     handy for rabbit inspection of dead cards. */
  
  unsigned int string_specs_offset;
  
#endif /* EEPROM_HEADER_ZE_VERSION >= 1 */
};

#define EEPROM_HEADER_ZE_SWAP(_hdr) do {				\
  struct eeprom_header_ze *swap_hdr = (_hdr);				\
									\
  swap_hdr->eeprom_len = htonl (swap_hdr->eeprom_len);			\
  swap_hdr->eeprom_segment_len = htonl (swap_hdr->eeprom_segment_len);	\
  swap_hdr->mcp1_offset = htonl (swap_hdr->mcp1_offset);		\
  swap_hdr->mcp2_offset = htonl (swap_hdr->mcp2_offset);		\
  swap_hdr->version = htonl (swap_hdr->version);			\
  if (swap_hdr->version >= 1) {						\
    swap_hdr->string_specs_offset					\
      = htonl (swap_hdr->string_specs_offset);				\
  }									\
} while (0)

/* Initialize the header __jump field. */

#define EEPROM_HEADER_ZE_INIT_JUMP(h) do {				\
  struct eeprom_header_ze *eeprom_header_ze_h = (h);			\
									\
  eeprom_header_ze_h->__jump[0] = 0x9;					\
  eeprom_header_ze_h->__jump[1] = 0x4;					\
  eeprom_header_ze_h->__jump[2] = 0xff;					\
  eeprom_header_ze_h->__jump[3] = 0x08;					\
  eeprom_header_ze_h->__jump[4] = 0x13;					\
  eeprom_header_ze_h->__jump[5]						\
    = (sizeof (struct eeprom_header_ze) - 6);				\
  eeprom_header_ze_h->__jump[6] = 0;					\
  eeprom_header_ze_h->__jump[7] = 0;					\
} while (0)

/* Verify that header has the jump sequence.  (This is used by
   the eeprom reprogrammer to verify the header is readable.) */

#define EEPROM_HEADER_ZE_MAGIC_OK(h) ((h)->__jump[0] == 0x09		\
				      && (h)->__jump[1] == 0x4		\
				      && (h)->__jump[2] == 0xff		\
				      && (h)->__jump[3] == 0x08		\
				      && (h)->__jump[4] == 0x13)

