/*************************************************************************
 * The contents of this file are subject to the MYRICOM ABSTRACTION      *
 * LAYER (MAL) SOFTWARE AND DOCUMENTATION LICENSE (the "License");       *
 * User may not use this file except in compliance with the License.     *
 * The full text of the License can found in LICENSE.TXT                 *
 *                                                                       *
 * Software distributed under the License is distributed on an "AS IS"   *
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See  *
 * the License for the specific language governing rights and            *
 * limitations under the License.                                        *
 *                                                                       *
 * Copyright 2003 - 2009 by Myricom, Inc.  All rights reserved.          *
 *************************************************************************/

#ifndef MAL_KERNEL
#include <stdio.h>
#if MAL_OS_LINUX
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <errno.h>
#elif MAL_OS_WINDOWS
#include <windows.h>
#elif MAL_OS_FREEBSD || MAL_OS_MACOSX
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#elif MAL_OS_SOLARIS
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stropts.h>
#include <sys/mman.h>
#include <sys/errno.h>
#elif MAL_OS_UDRV
#include <sys/socket.h>
#include <sys/un.h>
#include <alloca.h>
#include <unistd.h>
#include <sys/mman.h>
#include "mx_arch_comm.h"

static uint32_t mx_address_space;
char *mx__init_brk;
#else
#error
#endif

#include "mal.h"
#include "mal_io.h"
#include "mal_valgrind.h"

/* For systems w/o cloning devices, we use the convenction that:
 *  endpoint == -1 means open the generic board device (/dev/myrip0)
 *  endpoint == -2 means open the raw board device (/dev/myrir0) 
 */
MAL_FUNC(int)
mal_open(int unit, int endpoint, mal_handle_t *handle)
{
#if MAL_OS_LINUX || MAL_OS_FREEBSD || MAL_OS_MACOSX || MAL_OS_SOLARIS
  char buf[80];
  int rc;
  extern int errno;

#if MAL_OS_SOLARIS
  sprintf(buf, "/dev/myricp%d", unit);
#else
  sprintf(buf, "/dev/myrip%d", unit);
#endif

  if (MAL_OS_MACOSX)
    switch (endpoint) {
    case -2: 
      sprintf(buf, "/dev/myrir%d", unit); break;
    case -1: 
      sprintf(buf, "/dev/myrip%d", unit); break;
    default: 
      sprintf(buf, "/dev/myri%de%d", unit, endpoint); break;
    }

  rc = open(buf, O_RDWR);
  if (rc != -1) {
    *handle = rc;
    return 0;
  }

  /* try again with unprivledged devices */
  if (MAL_OS_MACOSX && endpoint >= 0) {
    sprintf(buf, "/dev/myri%de%d", unit, endpoint);
  } else {
#if MAL_OS_SOLARIS
    sprintf(buf, "/dev/myric%d", unit);
#else
    sprintf(buf, "/dev/myri%d", unit);
#endif
  }

  rc = open(buf, O_RDWR);
  if (rc != -1) {
    *handle = rc;
    return 0;
  }

  return errno;

#elif MAL_OS_WINDOWS
  char buf[80];
  DWORD last_error;

  /* TODO: Error handling. */
  if (endpoint == -1) {
    sprintf(buf, "\\\\.\\myri%d\\p", unit);
    *handle = CreateFile(buf, FILE_READ_ATTRIBUTES | SYNCHRONIZE, 0, NULL,
			 OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    if (*handle != INVALID_HANDLE_VALUE)
      return 0;
    else {
      sprintf(buf, "\\\\.\\myri%d", unit);
      *handle = CreateFile(buf, FILE_READ_ATTRIBUTES | SYNCHRONIZE, 0, NULL,
			   OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
      if (*handle != INVALID_HANDLE_VALUE)
	return 0;
      else {
	last_error = GetLastError();
	/* TODO: Magic number. */
	last_error &= ~((1 << 29) | (1 << 30) | (1 << 31));
	errno = last_error;
	return errno;
      }
    }
  }
  else if (endpoint == -2) {
    sprintf(buf, "\\\\.\\myri%d\\r", unit);
    *handle = CreateFile(buf, FILE_READ_ATTRIBUTES | SYNCHRONIZE, 0, NULL,
			 OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    if (*handle != INVALID_HANDLE_VALUE)
      return 0;
    else {
      last_error = GetLastError();
      /* TODO: Magic number. */
      last_error &= ~((1 << 29) | (1 << 30) | (1 << 31));
      errno = last_error;
      return errno;
    }
  }
  else {
    sprintf(buf, "\\\\.\\myri%d\\e%d", unit, endpoint);
    *handle = CreateFile(buf, FILE_READ_ATTRIBUTES | SYNCHRONIZE, 0, NULL,
			 OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    if (*handle != INVALID_HANDLE_VALUE)
      return 0;
    else {
      last_error = GetLastError();
      /* TODO: Magic number. */
      last_error &= ~((1 << 29) | (1 << 30) | (1 << 31));
      errno = last_error;
      return errno;
    }
  }

#elif MAL_OS_UDRV
  int rc, sock;
  struct sockaddr_un un;
  
  if (unit > 0)
    return EINVAL;
  sprintf(un.sun_path, MX_IOCTL_SOCK);
  sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock < 0)
    return errno;
  un.sun_family = AF_UNIX;
  rc = connect(sock,(struct sockaddr*)&un,sizeof(un));
  if (rc < 0) {
    close(sock);
    return errno;
  }
  *handle = sock;
  return 0;
#else
#error
#endif
}

MAL_FUNC(int)
mal_open_any_board(mal_handle_t *handle)
{
  int i, rc;
  int chrdev_exists = 0, driver_loaded = 0, perms_ok = 0;
  for (i = 0; i <= 32; i++) {
    if (!(rc = mal_open_board(i, handle)))
      return 0;
    else if (rc == ENOENT)
      break;
    else {
      chrdev_exists = 1;
      if (rc == ENODEV || rc == ENXIO)
	break;
      else {
	driver_loaded = 1;
	if ((rc != EACCES && rc != EPERM))
	  perms_ok = 1;
      }
    }
  }

  if (!chrdev_exists)
    return ENOENT;
  else if (!driver_loaded)
    return ENODEV;
  else if (!perms_ok)
    return EACCES;
  else
    return EBUSY;
}


MAL_FUNC(int)
mal_close(mal_handle_t handle)
{
#if MAL_OS_LINUX || MAL_OS_FREEBSD || MAL_OS_MACOSX || MAL_OS_SOLARIS
  extern int errno;
  
  if (close(handle))
    return errno;
  else
    return 0;
    
#elif MAL_OS_WINDOWS
  /* TODO: Error handling. */
  CloseHandle(handle);
  return 0;
#elif MAL_OS_UDRV
  shutdown(handle, SHUT_RDWR);
  if (close(handle))
    return errno;
  else
    return 0;
#else
#error
#endif
}

MAL_FUNC(int)
mal_ioctl(mal_handle_t handle, int cmd, void *buf, size_t bufsize)
{
#if MAL_OS_LINUX || MAL_OS_FREEBSD || MAL_OS_MACOSX || MAL_OS_SOLARIS
  int ret;
  MAL_VALGRIND_PRE_IOCTL_CHECK(cmd, buf);
  ret = ioctl(handle, cmd, buf);
  if (ret == 0) {
    MAL_VALGRIND_POST_IOCTL_CHECK(cmd, buf);
    return 0;
  } else
    return errno;
  
#elif MAL_OS_WINDOWS
  DWORD bytes;
  DWORD last_error;

  errno = 0;
  if (DeviceIoControl(handle, cmd, buf, (DWORD)bufsize, buf,
		      (DWORD)bufsize, &bytes, NULL) == 0) {
    last_error = GetLastError();
    /* TODO: Magic number. */
    last_error &= ~((1 << 29) | (1 << 30) | (1 << 31));
    return last_error;
  }
  else
    return 0;
  
#elif MAL_OS_UDRV
  struct iobuf {
    uint32_t cmd_status;
    char data[0];
  } *iobuf = 0;
  mx_nic_id_hostname_t *nic_host = buf;
  mx_set_hostname_t *set_host = buf;
  myri_raw_send_t *rsend = buf;
  myri_raw_next_event_t *rnext = buf;
  myri_get_logging_t *glog = buf;
  mx_get_route_table_t *tbl = buf;
  mx_get_eeprom_string_t *geeprom = buf;
  mx_reg_t rdma;
  int isize;
  int rc;

  /* handle scatter/gather cases of the driver/lib interface */
  switch (cmd) {
  case MX_REGISTER:
    isize = bufsize;
    rdma = *(mx_reg_t *)buf;
    mal_always_assert(rdma.nsegs == 1);
    mal_always_assert(rdma.segs.vaddr >= (size_t)mx__init_brk);
    rdma.segs.vaddr = ((uint64_t)mx_address_space << 32) +
      0x80000000 + rdma.segs.vaddr - (size_t)mx__init_brk;
    buf = &rdma;
    break;
  case MX_GET_ROUTE_TABLE:
    isize = bufsize;
    bufsize = 16384;
    break;
  case MX_GET_PRODUCT_CODE:
  case MX_GET_PART_NUMBER:
    isize = bufsize;
    bufsize = bufsize + 256;
    break;
  case MYRI_GET_COUNTERS_VAL:
    isize = bufsize;
    bufsize = 16384;
    break;
  case MYRI_GET_LOGGING:
    isize = bufsize;
    bufsize += glog->size;
    break;
  case MX_GET_MAPPER_MSGBUF:
  case MX_GET_MAPPER_MAPBUF:
    isize = 0;
    bufsize = 32768;
    break;
  case MX_GET_PEER_TABLE:
    bufsize = 16384;
    isize = 0;
    break;
  case MX_NIC_ID_TO_HOSTNAME:
  case MX_HOSTNAME_TO_NIC_ID:
    bufsize += nic_host->len;
    isize = bufsize;
    iobuf = alloca(bufsize + sizeof(*iobuf));
    memcpy(iobuf->data, buf, sizeof(*nic_host));
    memcpy(iobuf->data + sizeof(*nic_host), 
	   (void*)(uintptr_t)nic_host->va, nic_host->len);
    break;
  case MX_SET_HOSTNAME:
    bufsize += set_host->len;
    isize = bufsize;
    iobuf = alloca(bufsize + sizeof(*iobuf));
    memcpy(iobuf->data, buf, sizeof(*set_host));
    memcpy(iobuf->data + sizeof(*set_host), 
	   (void*)(uintptr_t)set_host->va, set_host->len);
    break;
  case MYRI_RAW_SEND:
    bufsize += rsend->data_length + rsend->route_length;
    isize = bufsize;
    iobuf = alloca(bufsize + sizeof(*iobuf));
    memcpy(iobuf->data, buf, sizeof(*rsend));
    memcpy(iobuf->data + sizeof(*rsend), 
	   (void*)(uintptr_t)rsend->route_pointer, rsend->route_length);
    memcpy(iobuf->data + sizeof(*rsend) + rsend->route_length,
	   (void*)(uintptr_t)rsend->data_pointer, rsend->data_length);
    break;
  case MYRI_RAW_GET_NEXT_EVENT:
    isize = bufsize;
    bufsize += 2048;
    break;
  case MYRI_GET_COUNTERS_STR:
    isize = bufsize;
    bufsize = 32768;
    break;
  default:
    isize = bufsize;
  }
  if (!iobuf) {
    iobuf = alloca(bufsize + sizeof(*iobuf));
    memcpy(iobuf->data, buf, isize);
  }
  iobuf->cmd_status = cmd;
  rc = udrv_send(handle, iobuf,bufsize + sizeof(*iobuf), 0);
  mal_always_assert(rc == bufsize + sizeof(*iobuf));
  rc = udrv_recv(handle, iobuf,bufsize + sizeof(*iobuf), 0);
  mal_always_assert(rc >= sizeof(*iobuf) && rc <= bufsize + sizeof(*iobuf));
  /* handle wierd driver/lib interface */
  if (iobuf->cmd_status == 0) {
    uint64_t old_field;
    switch (cmd) {
    case MX_GET_PRODUCT_CODE:
    case MX_GET_PART_NUMBER:
      strncpy((void*)(uintptr_t)geeprom->buffer, 
	      iobuf->data + sizeof(*geeprom), MYRI_MAX_STR_LEN);
      memcpy(geeprom, iobuf->data, sizeof(*geeprom));
      break;
    case MX_NIC_ID_TO_HOSTNAME:
    case MX_HOSTNAME_TO_NIC_ID:
      memcpy((void*)(uintptr_t)nic_host->va, 
	     iobuf->data + sizeof(*nic_host), nic_host->len);
      memcpy(nic_host, iobuf->data, sizeof(*nic_host));
      break;
    case MX_SET_HOSTNAME:
      memcpy((void*)(uintptr_t)set_host->va, 
	     iobuf->data + sizeof(*set_host), set_host->len);
      memcpy(set_host, iobuf->data, sizeof(*set_host));
      break;
    case MYRI_RAW_GET_NEXT_EVENT:
      old_field = rnext->recv_buffer;
      memcpy(rnext, iobuf->data, sizeof(*rnext));
      rnext->recv_buffer = old_field;
      if (rnext->status == MYRI_RAW_RECV_COMPLETE)
	memcpy((void*)(uintptr_t)rnext->recv_buffer, 
	       iobuf->data + sizeof(*rnext), rnext->recv_bytes);
      break;
    case MYRI_GET_LOGGING:
      memcpy((char*)(uintptr_t)glog->buffer, iobuf->data + sizeof(*glog), 
	     rc - sizeof(*iobuf) - sizeof(*glog));
      break;
    default:
      memcpy(buf, iobuf->data, rc - sizeof(*iobuf));
    }
  } else {
    fprintf(stderr,"command %x returned errno=%d:%s\n", cmd, 
	    iobuf->cmd_status, strerror(iobuf->cmd_status));
  }
  return iobuf->cmd_status;
#else
#error
#endif
}

MAL_FUNC(void *)
mal_mmap(void *start, size_t length, mal_handle_t handle, uintptr_t offset, int read_only)
{
#if MAL_OS_LINUX || MAL_OS_FREEBSD || MAL_OS_SOLARIS
  void *ptr;
  unsigned flags = PROT_READ;
  if (!read_only)
    flags |= PROT_WRITE;

  return mmap(start, length, flags, MAP_SHARED, handle, offset);
#elif MAL_OS_MACOSX || MAL_OS_WINDOWS
  myri_mmap_t x;
  int status;

  x.offset = (uint32_t)offset;
  x.len = (uint32_t)length;
  x.va = (uint64_t)(uintptr_t)start;
  x.requested_permissions = -1;  /* XXX does windows need this?? */
  if (mal_ioctl(handle, MYRI_MMAP, &x, sizeof(x)) == 0)
    return (void *)(uintptr_t)x.va;
  else
    return (void *)-1;

#elif MAL_OS_UDRV
  mx_mmapio_t x;
  int fd;
  char *ptr;
  unsigned long pad;

  memset(&x,0,sizeof(x));
  x.offset = (uint32_t)offset;
  x.len = (uint32_t)length;
  x.as = mx_address_space;
  if (mal_ioctl(handle, MYRI_MMAP, &x, sizeof(x)))
    return (void *)-1;
  mx_address_space = x.as;
  fd = open(x.outpath,O_RDWR);
  if (fd < 0)
    return (void *)-1;
  pad = 0;
  mal_always_assert(!start || pad == 0);
  ptr = mmap(start, x.len, PROT_READ | (read_only ? 0 : PROT_WRITE),
	     MAP_SHARED | MAP_FILE | (start ? MAP_FIXED : 0), 
	     fd, x.offset - pad);
  close(fd);
  return (uintptr_t)ptr == -1 ? ptr : ptr + pad;
#else
#error
#endif
}

MAL_FUNC(int)
mal_munmap(void *start, size_t length)
{
#if MAL_OS_LINUX || MAL_OS_FREEBSD || MAL_OS_SOLARIS || MAL_OS_UDRV
  return munmap(start, length);
#elif MAL_OS_MACOSX || MAL_OS_WINDOWS
  return 0;
#else
#error what OS !?
#endif
}
#endif
