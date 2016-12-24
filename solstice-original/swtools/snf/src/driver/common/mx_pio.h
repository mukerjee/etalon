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

/* This file implements PIO read and write macros for the driver.
   These macros simply do nothing more than endian conversion. */

#ifndef _mx_pio_h_
#define _mx_pio_h_

#include "mal.h"
#include "mal_stbar.h"
#include "mal_byteswap.h"

#include "mx_pio_def.h"


/****************************************************************
 * PIO with endian conversion
 ****************************************************************/

/****************
 * reads
 ****************/

static inline uint64_t
__mx_ntok_u64 (volatile uint64_t *ptr)
{
  return mx_ntoh_u64(MX_PIO_READ(ptr));
}

static inline uint32_t
__mx_ntok_u32(volatile uint32_t *ptr)
{
  return ntohl(MX_PIO_READ(ptr));
}

static inline uint16_t
__mx_ntok_u16(volatile uint16_t *ptr)
{
  return ntohs(MX_PIO_READ(ptr));
}

static inline uint8_t
__mx_ntok_u8(volatile uint8_t *ptr)
{
  return (MX_PIO_READ(ptr));
}

static inline uint32_t
__mx_letok_u32(volatile uint32_t *ptr)
{
  return mx_letoh_u32(MX_PIO_READ(ptr));
}

static inline uint16_t
__mx_letok_u16(volatile uint16_t *ptr)
{
  return mx_letoh_u32(MX_PIO_READ(ptr));
}

/****************
 * writes
 ****************/

static inline void
__mx_kton_u64(volatile uint64_t *ptr, uint64_t val)
{
  MX_PIO_WRITE(ptr, mx_hton_u64(val));
}

static inline void
__mx_kton_u32 (volatile uint32_t* ptr, uint32_t val)
{
  MX_PIO_WRITE(ptr, htonl(val));
}

static inline void
__mx_kton_u16(volatile uint16_t *ptr, uint16_t val)
{
  MX_PIO_WRITE(ptr, htons(val));
}

static inline void
__mx_kton_u8(volatile uint8_t *ptr, uint8_t val)
{
  MX_PIO_WRITE(ptr, val);
}

static inline void
__mx_ktole_u32 (volatile uint32_t* ptr, uint32_t val)
{
  MX_PIO_WRITE(ptr, mx_htole_u32(val));
}

static inline void
__mx_ktole_u16(volatile uint16_t *ptr, uint16_t val)
{
  MX_PIO_WRITE(ptr, mx_htole_u16(val));
}

/********************************
 * special registers
 ********************************/

/****************
 * reading
 ****************/

#define mx_rd_spec(is, fld, sfx)			\
__mx_ntok_##sfx (&(((mx_lanai_special_registers_t *)	\
		    is->lanai.special_regs)		\
		   ->read.fld))

#define mx_le_rd_spec(is, fld, sfx)			\
__mx_letok_##sfx (&(((mx_lanai_special_registers_t *)	\
		    is->lanai.special_regs)		\
		   ->read.fld))

#define mx_read_lanai_special_reg_u32(is, fld) mx_rd_spec(is, fld, u32)
#define mx_read_lanai_special_reg_u16(is, fld) mx_rd_spec(is, fld, u16)
#define mx_read_lanai_special_reg_u8(is, fld) mx_rd_spec(is, fld, u8)
#define mx_read_lanai_le_reg_u32(is, fld) mx_le_rd_spec(is, fld, u32)
#define mx_read_lanai_le_reg_u16(is, fld) mx_le_rd_spec(is, fld, u16)

/****************
 * writing
 ****************/

#define mx_wr_spec(is, fld, val, sfx)					\
__mx_kton_##sfx (&(((mx_lanai_special_registers_t *)			\
		    is->lanai.special_regs)				\
		   ->write.fld),					\
		 val)

#define mx_le_wr_spec(is, fld, val, sfx)					\
__mx_ktole_##sfx (&(((mx_lanai_special_registers_t *)			\
		    is->lanai.special_regs)				\
		   ->write.fld),					\
		 val)

#define mx_write_lanai_special_reg_u32(is,fld,val) mx_wr_spec(is,fld,val,u32)
#define mx_write_lanai_special_reg_u16(is,fld,val) mx_wr_spec(is,fld,val,u16)
#define mx_write_lanai_special_reg_u8( is,fld,val) mx_wr_spec(is,fld,val, u8)
#define mx_write_lanai_le_reg_u32(is,fld,val) mx_le_wr_spec(is,fld,val,u32)
#define mx_write_lanai_le_reg_u16(is,fld,val) mx_le_wr_spec(is,fld,val,u16)

/********************************
 * inline PIO copy
 ********************************/

#define MX_PIO_LAST  0x1
#define MX_PIO_FLUSH 0x2
#define MX_PIO_32BYTE_FLUSH 0x4
#define MX_PIO_32BIT_ALIGN 0x8

static inline void 
mx_pio_memcpy32(void *to, uint32_t *from32, size_t size, uint32_t flags)
{
  register volatile uint32_t *to32;
  uint32_t last, flush;
  uint32_t tmp;
  size_t i;

  mal_assert((size > 0) && !(size & 3) && !((uintptr_t) to & 3) 
	     && !((uintptr_t) from32 & 3));
  
  /* check if we have to order the last word */
  last = 0;
  if (flags & MX_PIO_LAST) {
    last = 1;
  }

  /* check if we have to flush the writes */
  flush = 0;
  if (flags & MX_PIO_FLUSH) {
    flush = 1;
  }

  to32 = (volatile uint32_t *) to;
  for (i = 0; i < (size / 4) - last; i++) {
    MX_PIO_WRITE(to32, *from32);
    to32++;
    from32++;
    if ((flags & MX_PIO_32BYTE_FLUSH) && (i & 7) == 7)
      MAL_STBAR();
  }
  
  if (last) {
    tmp = *from32;
    MAL_STBAR();
    MX_PIO_WRITE(to32, tmp);
  }
  if (flush) {
    MAL_STBAR();
  }
}

static inline void 
mx_pio_memcpy64(void *to, uint64_t *from64, size_t size, uint32_t flags)
{
  register volatile uint64_t *to64;
  uint32_t last, flush;
  uint64_t tmp;
  size_t i;

  mal_assert((size > 0) && !(size & 7) && !((uintptr_t) to & 7) 
             && !((uintptr_t) from64 & 7));
  
  /* check if we have to order the last word */
  last = 0;
  if (flags & MX_PIO_LAST) {
    last = 1;
  }

  /* check if we have to flush the writes */
  flush = 0;
  if (flags & MX_PIO_FLUSH) {
    flush = 1;
  }

  to64 = (volatile uint64_t *) to;
  for (i = 0; i < (size / 8) - last; i++) {
    MX_PIO_WRITE(to64, *from64);
    to64++;
    from64++;
    if ((flags & MX_PIO_32BYTE_FLUSH) && (i & 3) == 3)
      MAL_STBAR();
  }

  if (last) {
    tmp = *from64;
    MAL_STBAR();
    MX_PIO_WRITE(to64, tmp);
  }
  if (flush) {
    MAL_STBAR();
  }
}

static inline void
mx_pio_memcpy(void *to, void *from, size_t size, uint32_t flags)
{
#if MAL_OS_UDRV
  if (mx_lxgdb) {
    if ((flags & MX_PIO_LAST) == 0) {
      fake_pci_pio_write( to, from, size, __FILE__, __LINE__);
    } else {
      fake_pci_pio_write(to, from, size - sizeof(void*), __FILE__, __LINE__);
      fake_pci_pio_write((char*)to + size - sizeof(void*), (char*)from + size - sizeof(void*), 
			 sizeof(void*), __FILE__, __LINE__);
    }
    return;
  } else 
#endif
    if (sizeof (void *) == 4 || (flags & MX_PIO_32BIT_ALIGN)) {
      mx_pio_memcpy32(to, (uint32_t *)from, size, flags);
    } else {
      mx_pio_memcpy64(to, (uint64_t *)from, size, flags);
    }
}

static inline void
mx_pio_bzero(void *ptr, int len)
{
  /* Clear 8 bytes at a time, if possible. */
  
  if (((uintptr_t) ptr & 7) == 0) {
    while (len >= 8) {
#if !MAL_OS_UDRV
      MX_PIO_WRITE(((uint64_t *) ptr), (uint64_t) 0);
#endif
      ptr = (void *) ((char *) ptr + 8);
      len -= 8;
    }
  }
  
  /* Clear 4 bytes at a time, if possible. */
  
  if (((uintptr_t) ptr & 3) == 0) {
    while (len >= 4) {
      MX_PIO_WRITE(((uint32_t *) ptr), (uint32_t) 0);
      ptr = (void *) ((char *) ptr + 4);
      len -= 4;
    }
  }
  
  /* Clear remaining bytes, one at a time. */
  
  while (len >= 1) {
    MX_PIO_WRITE(((uint8_t *) ptr), (uint8_t) 0);
    ptr = (void *) ((char *) ptr + 1);
    len -= 1;
  }
}


static inline void
mx_pio_bcopy_read(const void *from, void *to, unsigned int len)
{
#if MAL_OS_UDRV
  if (mx_lxgdb) {
    while (len > 0) {
      unsigned chunk = 128 - ((size_t)from & 127);
      chunk = len > chunk ? chunk : len;
      fake_pci_pio_read((char *) from, (void *) to, chunk, 
			__FILE__, __LINE__);
      to = (char *)to + chunk;
      from = (char *)from + chunk;
      len -= chunk;
    }
    return;
  }
#endif
  bcopy(from, to, len);
}

static inline void
mx_pio_bcopy_write(const void *from, void *to, unsigned int len)
{
  const char *f;
  char *t, *limit;
  char a, b, c;

#if MAL_OS_UDRV && 0
  while (len > 0) {
    fake_pci_pio_write((char *) to, (void *) from, (len > 4096) ? 4096 : len, 
		       __FILE__, __LINE__);
    to = ((char *) to) + ((len > 4096) ? 4096 : len);
    from = ((char *) from) + ((len > 4096) ? 4096 : len);
    len = len - ((len > 4096) ? 4096 : len);
  }
  return;
#endif

  f = (const char *)from;
  t = (char *)to;
  limit = t + len;

  /* Copy just enough bytes so that 16*N bytes remain to be copied */

  switch (len & 15)
    {
    case 13:
      a = *f++;
      b = *f++;
      c = *f++;
      *t++ = a;
      *t++ = b;
      *t++ = c;
    case 10:
      a = *f++;
      b = *f++;
      c = *f++;
      *t++ = a;
      *t++ = b;
      *t++ = c;
    case 7:
      a = *f++;
      b = *f++;
      c = *f++;
      *t++ = a;
      *t++ = b;
      *t++ = c;
    case 4:
      a = *f++;
      b = *f++;
      c = *f++;
      *t++ = a;
      *t++ = b;
      *t++ = c;
    case 1:
      a = *f++;
      *t++ = a;
      break;
      
    case 14:
      a = *f++;
      b = *f++;
      c = *f++;
      *t++ = a;
      *t++ = b;
      *t++ = c;
    case 11:
      a = *f++;
      b = *f++;
      c = *f++;
      *t++ = a;
      *t++ = b;
      *t++ = c;
    case 8:
      a = *f++;
      b = *f++;
      c = *f++;
      *t++ = a;
      *t++ = b;
      *t++ = c;
    case 5:
      a = *f++;
      b = *f++;
      c = *f++;
      *t++ = a;
      *t++ = b;
      *t++ = c;
    case 2:
      a = *f++;
      b = *f++;
      *t++ = a;
      *t++ = b;
      break;
      
    case 15:
      a = *f++;
      b = *f++;
      c = *f++;
      *t++ = a;
      *t++ = b;
      *t++ = c;
    case 12:
      a = *f++;
      b = *f++;
      c = *f++;
      *t++ = a;
      *t++ = b;
      *t++ = c;
    case 9:
      a = *f++;
      b = *f++;
      c = *f++;
      *t++ = a;
      *t++ = b;
      *t++ = c;
    case 6:
      a = *f++;
      b = *f++;
      c = *f++;
      *t++ = a;
      *t++ = b;
      *t++ = c;
    case 3:
      a = *f++;
      b = *f++;
      c = *f++;
      *t++ = a;
      *t++ = b;
      *t++ = c;
    case 0:
      break;
    }

  /* copy the rest of the bytes */

  mal_assert(t <= limit);
  mal_assert(((limit - t) & 15) == 0);
  while (t < limit)
    {
      /* copy 16 bytes... fast! */
      /*     */ a = *f++;
      /*     */ b = *f++;
      /*     */ c = *f++;
      *t++ = a; a = *f++;
      *t++ = b; b = *f++;
      *t++ = c; c = *f++;
      *t++ = a; a = *f++;
      *t++ = b; b = *f++;
      *t++ = c; c = *f++;
      *t++ = a; a = *f++;
      *t++ = b; b = *f++;
      *t++ = c; c = *f++;
      *t++ = a; a = *f++;
      *t++ = b; b = *f++;
      *t++ = c; c = *f++;
      *t++ = a; a = *f++;
      *t++ = b;
      *t++ = c;
      *t++ = a;
    }
  mal_assert(t == limit);
}

#endif /* _mx_pio_h_ */
