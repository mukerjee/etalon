#ifndef _myri_pci_h_
#define _myri_pci_h_ 1

/* You must externally define pci_dev_t, pci_read_*() and pci_write_*()
   before including this file. */

#include <cap_vs_defs.h>
#include <cfg_defs.h>
#include <myri.h>


#define VS_INTERFACE_VERSION 17

/****************************************************************
 * Internal utility macros
 ****************************************************************/

/* Write a byte, else force an error return. */

#define MYRI_PCI_WRITE_BYTE(a, b, c) do {				\
  if (!myri_pci_write_byte ((a), (b), (c)))				\
    return MYRI_EPERM;							\
} while (0)
#define MYRI_PCI_WRITE_WORD(a, b, c) do {				\
  if (!myri_pci_write_word ((a), (b), (c)))				\
    return MYRI_EPERM;							\
} while (0)
#define MYRI_PCI_WRITE_LONG(a, b, c) do {				\
  if (!myri_pci_write_long ((a), (b), (c)))				\
    return MYRI_EPERM;							\
} while (0)

/* Concise access to fields within CAP_VS.  Succeed, else force an
   error return. */

#define VS_WRITE_BYTE(a, b, c) MYRI_PCI_WRITE_BYTE ((a), vs_offset + (b), (c))
#define VS_WRITE_WORD(a, b, c) MYRI_PCI_WRITE_WORD ((a), vs_offset + (b), (c))
#define VS_WRITE_LONG(a, b, c) MYRI_PCI_WRITE_LONG ((a), vs_offset + (b), (c))
#define VS_READ_BYTE(a, b) myri_pci_read_byte ((a), vs_offset + (b))
#define VS_READ_WORD(a, b) myri_pci_read_word ((a), vs_offset + (b))
#define VS_READ_LONG(a, b) myri_pci_read_long ((a), vs_offset + (b))

struct myri_pci_dev;

/****************************************************************
 * Capability searching
 ****************************************************************/

/* Find a PCI capability by ID and store its offset in *offset. */

static inline
int
myri_pci_find_cap (struct myri_pci_dev *dev, unsigned int capability_id,
		   unsigned int *offset) 
{
  unsigned int o;
  
  /* Verify the device has a capabilities list. */
  if (!CFG_CAPABILITIES_LIST (dev))
    return MYRI_EINVAL;
  /* Read the capabilities pointer. */
  o = CFG_CAPABILITIES_POINTER (dev) & 0xfc;
  /* Make sure we can read the capabilities region.  (We cannot
     on Linux if we are not root. */
  if (myri_pci_read_word (dev, o) == 0xffff)
    return MYRI_EPERM;
  /* Find matching capability. */
  while (o && myri_pci_read_byte (dev, o) != capability_id)
    o = myri_pci_read_byte (dev, o+1) & 0xfc;
  if (!o)
    return MYRI_EINVAL;
  *offset = o;
  return 0;
}

/* Handy macro to handle the NULL vs_offset case. */

#define COMPUTE_VS_OFFSET_IF_NEEDED(dev) do {				\
  int compute_vs_offset_error;						\
									\
  if (vs_offset)							\
    break;								\
  compute_vs_offset_error						\
    = myri_pci_find_cap ((dev), CAP_VS_CAPABILITY_ID_INITIAL_VAL,	\
			 &vs_offset);					\
  if (compute_vs_offset_error)						\
    return compute_vs_offset_error;					\
} while (0)

/****************************************************************
 * EEPROM programming
 ****************************************************************/

static inline
int
myri_pci_eeprom_read_ex (struct myri_pci_dev *dev,
			 unsigned int addr, void *_buf, unsigned int len,
			 unsigned int vs_offset /* optional, for
						   optimization */)
{
  unsigned char *buf = (unsigned char *)_buf;
  unsigned int data;
  unsigned int i, j;

  COMPUTE_VS_OFFSET_IF_NEEDED (dev);

  /* Issue the first read address. */
  
  VS_WRITE_BYTE (dev, 4*CAP_VS_EEPROM_READ_ADDR_WORD+3, addr>>16);
  VS_WRITE_BYTE (dev, 4*CAP_VS_EEPROM_READ_ADDR_WORD+2, addr>>8);
  VS_WRITE_BYTE (dev, 4*CAP_VS_EEPROM_READ_ADDR_WORD+1, addr);
  addr++;

  /* Issue all the reads, and harvest the results every 4th issue. */
  
  for (i=0; i<len; ++i, addr++) {
    
    /* Issue the next read address, updating only the bytes that need
       updating.  We always update the LSB, which triggers the read. */
    
    if ((addr & 0xff) == 0) {
      if ((addr & 0xffff) == 0)
	VS_WRITE_BYTE (dev, 4*CAP_VS_EEPROM_READ_ADDR_WORD+3, addr>>16);
      VS_WRITE_BYTE (dev, 4*CAP_VS_EEPROM_READ_ADDR_WORD+2, addr>>8);
    }
    VS_WRITE_BYTE (dev, 4*CAP_VS_EEPROM_READ_ADDR_WORD+1, addr);

    /* If 4 data bytes are available, read them with a single read. */
    
    if ((i & 3) == 3) {
      data = VS_READ_LONG (dev, 4*CAP_VS_EEPROM_READ_DATA_WORD);
      for (j=0; j<4; j++) {
	buf[i-j] = data;
	data >>= 8;
      }
    }
  }

  /* Harvest any remaining results. */
  
  if ((i & 3) != 0) {
    data = VS_READ_LONG (dev, 4*CAP_VS_EEPROM_READ_DATA_WORD);
    for (j=1; j<=(i&3); j++) {
      buf[i-j] = data;
      data >>= 8;
    }
  }
  return 0;
}

static inline
int
myri_pci_eeprom_read (struct myri_pci_dev *dev,
		      unsigned int addr, void *_buf, unsigned int len)
{
  return myri_pci_eeprom_read_ex (dev, addr, _buf, len, 0);
}

static inline
int
myri_pci_eeprom_write_ex (struct myri_pci_dev *dev,
			  unsigned int addr, const void *_buf, unsigned int len,
			  unsigned int vs_offset /* optional, for
						     optimization */)
{
  const unsigned char *buf = (const unsigned char *)_buf;
  unsigned int i;
  unsigned int nic_addr = ~addr; /* force initial write of entire address */
  int retries;

  COMPUTE_VS_OFFSET_IF_NEEDED (dev);

  /* Push the data a byte at a time. */
  if (VS_READ_BYTE(dev, VS_INTERFACE_VERSION) >= 1) {
    VS_WRITE_BYTE (dev, 4*CAP_VS_DEBUG_MODE_WORD, CFG_VS_DEBUG_MODE_EepromStreamWrite);
    
    VS_WRITE_BYTE (dev, 4 * CAP_VS_DEBUG_ADDR_WORD+3, addr >> 24);
    VS_WRITE_BYTE (dev, 4 * CAP_VS_DEBUG_ADDR_WORD+2, addr >> 16);
    VS_WRITE_BYTE (dev, 4 * CAP_VS_DEBUG_ADDR_WORD+1, addr >> 8);
    VS_WRITE_BYTE (dev, 4 * CAP_VS_DEBUG_ADDR_WORD+0, addr >> 0);

    for (i=0; i<len; i++, addr++)
      VS_WRITE_BYTE (dev, 4 * CAP_VS_DEBUG_DATA_WORD+0,  buf[i]);

    VS_WRITE_LONG (dev, 4 * CAP_VS_DEBUG_ADDR_WORD, 0xffffffff);
    return 0;
  }
  for (i=0; i<len; i++, addr++) {
    unsigned int read_byte;

    /* Skip 0xff writes, which are used to erase segments. */
    
    if (buf[i] == 0xff)
      continue;
    
    /* Update the NIC address as needed.  This will be required only
       for the first iteration, and any iteration after skipping 0xff
       bytes. */
    
    if (nic_addr != addr) {
      if (((nic_addr >> 16) & 0xff) != ((addr >> 16) & 0xff))
	VS_WRITE_BYTE (dev, 4*CAP_VS_EEPROM_WRITE_ADDR_WORD+3, addr>>16);
      if (((nic_addr >> 8) & 0xff) != ((addr >> 8) & 0xff))
	VS_WRITE_BYTE (dev, 4*CAP_VS_EEPROM_WRITE_ADDR_WORD+2, addr>>8);
      if (((nic_addr >> 0) & 0xff) != ((addr >> 0) & 0xff))
	VS_WRITE_BYTE (dev, 4*CAP_VS_EEPROM_WRITE_ADDR_WORD+1, addr>>0);
    }
    
    /* Write the byte.  The NIC assumes this increments the write addr,
       and automatically performs a read from the address
       into CAP_VS_EEPROM_READ_DATA. */
    
    VS_WRITE_BYTE (dev, 4*CAP_VS_EEPROM_WRITE_DATA_WORD, buf[i]);
    nic_addr = addr + 1;	/* Track the NIC's prediction. */

    /* Wait for the byte to be programmed.

       During programming bit 6 is toggling and bit 7 is the
       complement of what we are programming.  After programming, the
       programmed value is returned.  Programming only clears bits.

       The upshot of all this is that we are successful when we read
       the desired-programmed value, and we fail when two successive
       reads return the same value, indicating the user tried to set
       an already cleared bit. */

    read_byte = VS_READ_BYTE (dev, 4*CAP_VS_EEPROM_READ_DATA_WORD+0);
    retries = 0;
    while (read_byte != buf[i]) {	/* didn't succeed on first read. */
      unsigned int prev_read_byte;

      /* Reread the byte. */
      prev_read_byte = read_byte;
      VS_WRITE_BYTE (dev, 4*CAP_VS_EEPROM_READ_ADDR_WORD+1, addr);
      read_byte = VS_READ_BYTE (dev, 4*CAP_VS_EEPROM_READ_DATA_WORD+0);
      retries += 1;

      /* Check for failure. */
      if (read_byte == prev_read_byte && retries > 100) {
	fprintf(stderr, "ERROR: burning 0x%02x at addr 0x%x, retries=%d, prev_read/read=0x%02x/0x%02x\n",
		buf[i], addr, retries, prev_read_byte, read_byte);
	return EINVAL;
      }
    }
  }
  return 0;
}

static inline
int
myri_pci_eeprom_write (struct myri_pci_dev *dev,
		       unsigned int addr, const void *_buf, unsigned int len)
{
  return myri_pci_eeprom_write_ex (dev, addr, _buf, len, 0);
}

static inline
int
myri_pci_eeprom_erase_sector_ex (struct myri_pci_dev *dev,
				 unsigned int addr,
				 unsigned int vs_offset /* optional, for
							   optimization */)
{
  unsigned int verify;

  COMPUTE_VS_OFFSET_IF_NEEDED (dev);
  
  /* Initate a sector erase and arrange for 2 reads to appear
     in CAP_VS_EEPROM_READ_DATA, spinning until the erase initiates. */

  VS_WRITE_BYTE (dev, 4*CAP_VS_EEPROM_WRITE_ADDR_WORD+3, addr>>16);
  VS_WRITE_BYTE (dev, 4*CAP_VS_EEPROM_WRITE_ADDR_WORD+2, addr>>8);
  VS_WRITE_BYTE (dev, 4*CAP_VS_EEPROM_WRITE_ADDR_WORD+1, addr>>0);
  VS_WRITE_BYTE (dev, 4*CAP_VS_EEPROM_WRITE_ADDR_WORD+0, 0xff);

  /* Spin until programming completes (data becomes 0xff) */

  verify = VS_READ_LONG (dev, 4*CAP_VS_EEPROM_READ_DATA_WORD);
  while ((verify & 0xff) != 0xff) {
    VS_WRITE_BYTE (dev, 4*CAP_VS_EEPROM_READ_ADDR_WORD+1, addr>>0);
    verify = VS_READ_LONG (dev, 4*CAP_VS_EEPROM_READ_DATA_WORD);
  }

  return 0;
}

static inline
int
myri_pci_eeprom_erase_sector (struct myri_pci_dev *dev, unsigned int addr)
{
  return myri_pci_eeprom_erase_sector_ex (dev, addr, 0);
}

/****************************************************************
 * Debug interface (for accessing SRAM and special registers)
 ****************************************************************/

/* Set the debug mode for the new transaction. */

#define MYRI_PCI_DEBUG_MODE_WRITE(d, _mode)				\
/**/ VS_WRITE_BYTE ((d), 4*CAP_VS_DEBUG_MODE_WORD, (_mode));

/* Read data in significance order. */

#define MYRI_PCI_DEBUG_READ_DATA(d)					\
/**/ VS_READ_LONG ((d), 4*CAP_VS_DEBUG_DATA_WORD)

/* Write data in significance order.  Since we can rely only on byte
   writes, write only the required data bytes.  Force a return on
   error. */

#define MYRI_PCI_DEBUG_DATA_WRITE(_d, _mode, _val) do {			\
  unsigned int VAL = (_val);						\
									\
  switch (_mode) {							\
  case CFG_VS_DEBUG_MODE_Write32:					\
    VS_WRITE_BYTE (d, 4*CAP_VS_DEBUG_DATA_WORD+3, VAL>>24);		\
    VS_WRITE_BYTE (d, 4*CAP_VS_DEBUG_DATA_WORD+2, VAL>>16);		\
  case CFG_VS_DEBUG_MODE_Write16:					\
    VS_WRITE_BYTE (d, 4*CAP_VS_DEBUG_DATA_WORD+1, VAL>>8);		\
  case CFG_VS_DEBUG_MODE_Write8:					\
    VS_WRITE_BYTE (d, 4*CAP_VS_DEBUG_DATA_WORD, VAL);			\
    break;								\
  default:								\
    return MYRI_EINVAL;							\
  }									\
} while (0)

/* Write the debug address, possibly triggering a write. */

#define MYRI_PCI_DEBUG_ADDR_WRITE(_d, _addr, _prev_addr) do {		\
  unsigned int myri_pci_addr, myri_pci_prev_addr;			\
  struct myri_pci_dev *wd;							\
									\
  wd = (_d);								\
  myri_pci_addr = (_addr);						\
  myri_pci_prev_addr = (_prev_addr);					\
  if ((myri_pci_addr ^ myri_pci_prev_addr) & 0xff000000)		\
    VS_WRITE_BYTE (wd, 4*CAP_VS_DEBUG_ADDR_WORD+3, myri_pci_addr>>24);	\
  if ((myri_pci_addr ^ myri_pci_prev_addr) & 0xff0000)			\
    VS_WRITE_BYTE (wd, 4*CAP_VS_DEBUG_ADDR_WORD+2, myri_pci_addr>>16);	\
  if ((myri_pci_addr ^ myri_pci_prev_addr) & 0xff00)			\
    VS_WRITE_BYTE (wd, 4*CAP_VS_DEBUG_ADDR_WORD+1, myri_pci_addr>>8);	\
  /* Always write the LSByte, to trigger any access. */			\
  VS_WRITE_BYTE (wd, 4*CAP_VS_DEBUG_ADDR_WORD, myri_pci_addr);		\
} while (0)

#define MYRI_PCI_DEBUG_ADDR_READ(_d) \
/**/ VS_READ_LONG ((_d), 4*CAP_VS_DEBUG_ADDR_WORD)

/* Read 1, 2, or 4 bytes in significance order. */

static inline
int
myri_pci_debug_read_atom (struct myri_pci_dev *d, unsigned int vs_offset,
			  unsigned int addr, unsigned int bytes, void *val)
{
  COMPUTE_VS_OFFSET_IF_NEEDED (d);

  if (bytes == 1) {
    MYRI_PCI_DEBUG_MODE_WRITE (d, CAP_VS_DEBUG_MODE_Read8);
    MYRI_PCI_DEBUG_ADDR_WRITE (d, addr, ~addr);
    *(char *)val = MYRI_PCI_DEBUG_READ_DATA (d);
  } else if (bytes == 2) {
    MYRI_PCI_DEBUG_MODE_WRITE (d, CAP_VS_DEBUG_MODE_Read16);
    MYRI_PCI_DEBUG_ADDR_WRITE (d, addr, ~addr);
    *(short *)val = MYRI_PCI_DEBUG_READ_DATA (d);
  } else if (bytes == 4) {
    MYRI_PCI_DEBUG_MODE_WRITE (d, CAP_VS_DEBUG_MODE_Read32);
    MYRI_PCI_DEBUG_ADDR_WRITE (d, addr, ~addr);
    *(int *)val = MYRI_PCI_DEBUG_READ_DATA (d);
  } else {
    return MYRI_EINVAL;
  }
  
  return 0;
}

/* Write 1, 2, or 4 bytes in significance order. */

static inline
int
myri_pci_debug_write_atom (struct myri_pci_dev *d, unsigned int vs_offset,
			   unsigned int addr, unsigned int bytes,
			   unsigned int val)
{
  unsigned char mode;

  COMPUTE_VS_OFFSET_IF_NEEDED (d);
  
  /* Determine the write mode. */
  
  if (bytes == 1)
    mode = CFG_VS_DEBUG_MODE_Write8;
  else if (bytes == 2)
    mode = CFG_VS_DEBUG_MODE_Write16;
  else if (bytes == 4)
    mode = CFG_VS_DEBUG_MODE_Write32;
  else
    return MYRI_EINVAL;

  /* Perform the write, using as few byte writes as possible. */
  
  MYRI_PCI_DEBUG_MODE_WRITE (d, mode);
  MYRI_PCI_DEBUG_ADDR_WRITE (d, addr, ~addr);
  MYRI_PCI_DEBUG_DATA_WRITE (d, mode, val);

  return 0;
}

/* Copy bytes to the LANai.  Write bytes as atomically as possible
   for safety, but maintain byte order. */

static inline
int
myri_pci_debug_write (struct myri_pci_dev *d, unsigned int vs_offset,
		      unsigned int addr, const void *_ptr, unsigned int len) 
{
  unsigned char *ptr = (unsigned char *)_ptr;
  
  if (len == 0)
    return 0;

  COMPUTE_VS_OFFSET_IF_NEEDED (d);

  MYRI_PCI_DEBUG_MODE_WRITE (d, CAP_VS_DEBUG_MODE_Write8);
  MYRI_PCI_DEBUG_ADDR_WRITE (d, addr, ~addr);

  if (len && (addr & 1)) {
    MYRI_PCI_DEBUG_DATA_WRITE (d, CAP_VS_DEBUG_MODE_Write8, ptr[0]);
    ptr++, addr++, len--;
  }
  if (len && (addr & 2)) {
    MYRI_PCI_DEBUG_MODE_WRITE (d, CAP_VS_DEBUG_MODE_Write16);
    MYRI_PCI_DEBUG_DATA_WRITE (d, CAP_VS_DEBUG_MODE_Write16, ptr[0]<<8|ptr[1]);
    ptr+=2, addr+=2, len-=2;
  }
  if (len >= 4) {
    MYRI_PCI_DEBUG_MODE_WRITE (d, CAP_VS_DEBUG_MODE_Write32);
    do {
      MYRI_PCI_DEBUG_DATA_WRITE (d, CAP_VS_DEBUG_MODE_Write16,
				 ptr[0]<<24|ptr[1]<<16|ptr[2]<<8|ptr[3]);
      ptr+=4, addr+=4, len-=4;
    } while (len >= 4);
  }
  if (len >= 2) {
    MYRI_PCI_DEBUG_MODE_WRITE (d, CAP_VS_DEBUG_MODE_Write16);
    MYRI_PCI_DEBUG_DATA_WRITE (d, CAP_VS_DEBUG_MODE_Write16, ptr[0]<<8|ptr[1]);
    ptr+=2, addr+=2, len-=2;
  }
  if (len >= 1) {
    MYRI_PCI_DEBUG_MODE_WRITE (d, CAP_VS_DEBUG_MODE_Write8);
    MYRI_PCI_DEBUG_DATA_WRITE (d, CAP_VS_DEBUG_MODE_Write8, ptr[0]);
    ptr++, addr++, len--;
  }
  return 0;
}

/* Read bytes from the lanai.  Read as atomically as possible for
   safety, but maintain byte order. */

static inline
int
myri_pci_debug_read (struct myri_pci_dev *d, unsigned int vs_offset,
		     unsigned int addr, void *_ptr, unsigned int len)
{
  unsigned char *ptr = (unsigned char *)_ptr;
  unsigned int prev_addr = ~addr;

  COMPUTE_VS_OFFSET_IF_NEEDED (d);
  
  if (len && (addr & 1)) {
    MYRI_PCI_DEBUG_MODE_WRITE (d, CAP_VS_DEBUG_MODE_Read8);
    MYRI_PCI_DEBUG_ADDR_WRITE (d, addr, prev_addr);
    prev_addr = addr;
    ptr[0] = MYRI_PCI_DEBUG_READ_DATA (d);
    ptr++, addr++, len--;
  }
  if (len && (addr & 2)) {
    unsigned short bytes;
    MYRI_PCI_DEBUG_MODE_WRITE (d, CAP_VS_DEBUG_MODE_Read16);
    MYRI_PCI_DEBUG_ADDR_WRITE (d, addr, prev_addr);
    prev_addr = addr;
    bytes = MYRI_PCI_DEBUG_READ_DATA (d);
    ptr[0] = bytes>>8;
    ptr[1] = bytes;
    ptr+=2, addr+=2, len-=2;
  }
  if (len >= 4) {
    unsigned int bytes;
    MYRI_PCI_DEBUG_MODE_WRITE (d, CFG_VS_DEBUG_MODE_Read32);
    do {
      MYRI_PCI_DEBUG_ADDR_WRITE (d, addr, prev_addr);
      prev_addr = addr;
      bytes = MYRI_PCI_DEBUG_READ_DATA (d);
      ptr[0] = bytes>>24;
      ptr[1] = bytes>>16;
      ptr[2] = bytes>>8;
      ptr[3] = bytes;
      ptr+=4, addr+=4, len-=4;
    } while (len >= 4);
  }
  if (len >= 2) {
    unsigned short bytes;
    MYRI_PCI_DEBUG_MODE_WRITE (d, CAP_VS_DEBUG_MODE_Read16);
    MYRI_PCI_DEBUG_ADDR_WRITE (d, addr, prev_addr);
    prev_addr = addr;
    bytes = MYRI_PCI_DEBUG_READ_DATA (d);
    ptr[0] = bytes>>8;
    ptr[1] = bytes;
    ptr+=2, addr+=2, len-=2;
  }
  if (len >= 1) {
    MYRI_PCI_DEBUG_MODE_WRITE (d, CAP_VS_DEBUG_MODE_Read8);
    MYRI_PCI_DEBUG_ADDR_WRITE (d, addr, prev_addr);
    prev_addr = addr;
    ptr[0] = MYRI_PCI_DEBUG_READ_DATA (d);
    ptr++, addr++, len--;
  }
  
  return 0;
}

/* Find the addr,len of a resource in the NIC, triggering an update
   of the resource, if appropriate. */

static inline
int
myri_pci_locate (struct myri_pci_dev *d,
		 unsigned int vs_offset, /* optional */
		 unsigned int resource_id,
		 unsigned int *addr, unsigned int *len) 
{
  COMPUTE_VS_OFFSET_IF_NEEDED (d);
  
  MYRI_PCI_DEBUG_MODE_WRITE (d, CAP_VS_DEBUG_MODE_Locate);
  MYRI_PCI_DEBUG_ADDR_WRITE (d, resource_id, ~resource_id);
  *addr = MYRI_PCI_DEBUG_ADDR_READ (d);
  *len = MYRI_PCI_DEBUG_READ_DATA (d);
  return *addr ? 0 : MYRI_EINVAL;
}

/* Take a snapshot of a resource in the NIC using the COPY engine. */
   
static inline
int
myri_pci_snapshot (struct myri_pci_dev *d,
		   unsigned int vs_offset, /* optional */
		   unsigned int *addr, unsigned int *len)
{
  COMPUTE_VS_OFFSET_IF_NEEDED (d);
  
  /* Request a snapshot. */
  MYRI_PCI_DEBUG_MODE_WRITE (d, CAP_VS_DEBUG_MODE_Snapshot);
  MYRI_PCI_DEBUG_ADDR_WRITE (d, *addr, ~*addr);
  MYRI_PCI_DEBUG_DATA_WRITE (d, CAP_VS_DEBUG_MODE_Write32, *len);
  /* Return the location and length of the snapshot.  (Snapshots
     may be truncated if the snapshot buffer is too small.) */
  *len = MYRI_PCI_DEBUG_READ_DATA (d);
  *addr = MYRI_PCI_DEBUG_ADDR_READ (d);
  return *len ? 0 : MYRI_EINVAL;
}

#endif /* _myri_pci_h_ */
