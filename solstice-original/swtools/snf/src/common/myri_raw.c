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
 * Copyright 2005 - 2010 by Myricom, Inc.  All rights reserved.          *
 *************************************************************************/

#include "mal.h"
#include "mal_io.h"
#include "mal_thread.h"
#include "myri_raw.h"
#include "mcp_config.h"


#define MYRI_RAW_NUM_TRANSMITS (MCP_KREQQ_CNT - 2) /* XXX FIXME XXX */

struct myri_raw_endpoint
{
  MAL_MUTEX_T lock;
  MAL_MUTEX_T next_event_lock;
  mal_handle_t handle;
  int board_num;
  int board_type;
  int pending_sends;
};


#ifndef MAL_KERNEL
#ifdef _WIN32
MAL_FUNC(HANDLE) 
#else
MAL_FUNC(int)
#endif
myri_raw_handle(myri_raw_endpoint_t ep)
{
  return ep->handle;
}
#endif

MAL_FUNC(int)
myri_raw_open_endpoint(uint32_t board_number, 
		       myri_raw_endpoint_t *endpoint)

{
  struct myri_raw_endpoint *ep;
  uint32_t i;
  mal_handle_t handle = 0;
  int rc;

  ep = mal_calloc(1, sizeof(*ep));
  if (ep == NULL) {
    rc = ENOMEM;
    goto abort_with_nothing;
  }

  rc = mal_open(board_number, -2, &handle);
  if (rc)
    goto abort_with_ep;

#ifdef MAL_KERNEL
  rc = myri_klib_set_endpoint(&handle, board_number, 0, NULL, MX_SET_RAW);
#else
  rc = mal_ioctl(handle, MX_SET_RAW, 0, 0);
#endif
  if (rc != 0) {
    goto abort_with_handle;
  }

  ep->handle = handle;
  ep->board_num = board_number;

  /* get board type */
  i = board_number;
  rc = mal_ioctl(ep->handle, MYRI_GET_BOARD_TYPE, &i, sizeof(uint32_t));
  if (rc)
    goto abort_with_handle;
  ep->board_type = i;

  MAL_MUTEX_INIT(&ep->lock);
  MAL_MUTEX_INIT(&ep->next_event_lock);

  *endpoint = ep;
  return rc;

 abort_with_handle:
  mal_close(handle);

 abort_with_ep:
  mal_free(ep);

 abort_with_nothing:
  return rc;
}

MAL_FUNC(int)
myri_raw_close_endpoint(myri_raw_endpoint_t endpoint)
{
  
  MAL_MUTEX_DESTROY(&endpoint->lock);
  MAL_MUTEX_DESTROY(&endpoint->next_event_lock);
  mal_free(endpoint);
  
  return 0;
}


MAL_FUNC(int)
myri_raw_send(myri_raw_endpoint_t endpoint,
	      uint32_t physical_port,
	      void *route_pointer,
	      uint32_t route_length,
	      void *send_buffer,
	      uint32_t data_length,
	      void *context)
{
  myri_raw_send_t x;
  int rc;
  
  MAL_MUTEX_LOCK(&endpoint->lock);
  if (endpoint->pending_sends >= MYRI_RAW_NUM_TRANSMITS) {
    rc = EBUSY;
    goto abort;
  }

  /* build the raw send */
  x.physical_port = physical_port;
  x.route_pointer = (uintptr_t)route_pointer;
  x.route_length = route_length;
  x.data_pointer = (uintptr_t)send_buffer;
  x.data_length = data_length;
  x.context = (uintptr_t)context;

  rc = mal_ioctl(endpoint->handle, MYRI_RAW_SEND, &x, sizeof(x));
  if (!rc)
    endpoint->pending_sends++;

  abort:
  MAL_MUTEX_UNLOCK(&endpoint->lock);
  return rc;
}

MAL_FUNC(int)
myri_raw_next_event(myri_raw_endpoint_t endpoint,
		    uint32_t *incoming_port,
		    void **context,
		    void *recv_buffer,
		    uint32_t *recv_bytes,
		    uint32_t timeout_ms,
		    myri_raw_status_t *status
#if MYRI_ENABLE_PTP
		    , uint64_t *timestamp_ns
#endif
		    )
{
  myri_raw_next_event_t e;
  int rc;

  e.timeout = timeout_ms;
  e.recv_buffer = (uint64_t)(uintptr_t)recv_buffer;
  e.recv_bytes = *recv_bytes;
  MAL_MUTEX_LOCK(&endpoint->next_event_lock);
  rc = mal_ioctl(endpoint->handle, MYRI_RAW_GET_NEXT_EVENT, &e, sizeof(e));
  MAL_MUTEX_UNLOCK(&endpoint->next_event_lock);
  if (rc != 0)
    goto abort;

  *status = e.status;
  switch (e.status) {
  case MYRI_RAW_NO_EVENT:
    /* nothing to do here, move along... */
    break;
  case MYRI_RAW_SEND_COMPLETE:
    *context = (void *)(uintptr_t)e.context;
#if MYRI_ENABLE_PTP
    *timestamp_ns = e.timestamp_ns;
#endif
    MAL_MUTEX_LOCK(&endpoint->lock);
    endpoint->pending_sends--;
    MAL_MUTEX_UNLOCK(&endpoint->lock);
    break;
  case MYRI_RAW_RECV_COMPLETE:
    *incoming_port = e.incoming_port;
    *recv_bytes = e.recv_bytes;
    break;
  default:
    /* NIC is dead... */
    break;
  }

 abort:
  return rc;
}

#if MAL_OS_WINDOWS
MAL_FUNC(int)
myri_raw_enable_hires_timer(myri_raw_endpoint_t endpoint)
{
  return 1;
}

MAL_FUNC(int)
myri_raw_disable_hires_timer(myri_raw_endpoint_t endpoint)
{
  return 1;
}
#endif


