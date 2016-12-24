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

#ifndef MX_RAW_H
#define MX_RAW_H

#include "mal.h"

#define MYRI_RAW_NO_EVENT      0
#define MYRI_RAW_SEND_COMPLETE 1
#define MYRI_RAW_RECV_COMPLETE 2

/*
 * Linux and MacOSX now support polling on raw endpoint
 */
#if (MAL_OS_LINUX || MAL_OS_MACOSX)
#define MX_RAW_POLL_SUPPORTED 1
#endif

#ifdef __cplusplus
extern "C"
{
#if 0
}
#endif
#endif

typedef struct myri_raw_endpoint * myri_raw_endpoint_t;

/****************************************/
/* myri_raw_open_endpoint:              */
/****************************************/
/*
 Give the handle of the raw endpoint for use with
    mx__driver_interface.h
*/

#ifdef _WIN32
MAL_FUNC(HANDLE) 
#else
MAL_FUNC(int)
#endif
myri_raw_handle(myri_raw_endpoint_t ep);

/****************************************/
/* myri_raw_open_endpoint:              */
/****************************************/

/*
 Opens the raw endpoint.  There is one raw endpoint per instance,
 and by default only a priviledged application may open it. 
*/

MAL_FUNC(int) myri_raw_open_endpoint(uint32_t board_number,
				     myri_raw_endpoint_t *endpoint);

/****************************************/
/* myri_raw_close_endpoint:             */
/****************************************/
/*
 Closes a raw endpoint 
*/
MAL_FUNC(int) myri_raw_close_endpoint(myri_raw_endpoint_t endpoint);

/****************************************/
/* myri_raw_send:                       */
/****************************************/

/* 
  Sends a raw message of length data_length from data_pointer out
  physical_port, using the route specified by route_pointer and
  route_length. data_length must not exceed MYRI_RAW_MTU (currently 1KB).

  All messages are buffered, so the send_buffer can be recycled
  by the caller immediately after this function completes.

  The amount of buffer space is finite. There may be at most
  MYRI_RAW_NUM_TRANSMITS (currently 64) transmits outstanding at one
  time.  Buffer space is reclaimed when a send completion event
  is received via myri_raw_next_event().

  Tag may be used to identify which send completed.

*/

MAL_FUNC(int) myri_raw_send(myri_raw_endpoint_t endpoint,
			    uint32_t physical_port,
			    void *route_pointer,
			    uint32_t route_length,
			    void *send_buffer,
			    uint32_t buffer_length,
			    void *context);

/****************************************/
/* myri_raw_next_event:                 */
/****************************************/

typedef int myri_raw_status_t;

/* 
   Obtains the next raw event (received message or
   send completion), or reports there is no event ready.
   The caller should set recv_bytes to the maximum length
   receive he is prepared to handle.

   Blocks for up to timeout_ms (could be much more if
   the high resolution timer has not been enabled) waiting
   for an event.   If timeout_ms is zero, then it just
   obtains the next raw event, returning immediately
   even if no events are pending.

   The following are modified after a receive completion:
       incoming_port, recv_buffer, recv_bytes

   A send completion implicitly returns a transmit token
   to the caller.  Sends are recycled in order.

   myri_raw_status_t is used to determine the type of 
   event (if any) which occured.
*/

MAL_FUNC(int) myri_raw_next_event(myri_raw_endpoint_t endpoint,
				  uint32_t *incoming_port,
				  void **context,
				  void *recv_buffer,
				  uint32_t *recv_bytes,
				  uint32_t timeout_ms,
				  myri_raw_status_t *status
#if MYRI_ENABLE_PTP
				  , uint64_t *timestamp_ns
#endif
				  );

#if MAL_OS_WINDOWS
/****************************************/
/* myri_raw_enable_hires_timer:         */
/* myri_raw_disable_hires_timer:        */
/****************************************/

/* Enable and disable the high resolution timer.  The
   high resolution timer is resource intensive and should
   be disabled whenever possible
 */
MAL_FUNC(int) myri_raw_enable_hires_timer(myri_raw_endpoint_t endpoint);

MAL_FUNC(int) myri_raw_disable_hires_timer(myri_raw_endpoint_t endpoint);
#endif




#endif /* MX_RAW_H*/
