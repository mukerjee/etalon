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
 * Copyright 2003 - 2010 by Myricom, Inc.  All rights reserved.          *
 *************************************************************************/

#include "mx_arch.h"
#include "mx_instance.h"
#include "mx_malloc.h"
#include "mx_misc.h"
#include "mx_pio.h"
#include "mcp_config.h"
#include "mal_stbar.h"


int
mx_mcp_command(mx_instance_state_t *is, uint8_t cmd, uint32_t index, 
	       uint32_t data0, uint32_t data1, uint32_t *result)
{
  unsigned long flags;
  mcp_cmd_done_t *done;
  int sleep, status = 0;

  flags = 0; /* useless initialization to pacify -Wunused on platforms
		where flags are not used */

  done = (mcp_cmd_done_t *)(is->sysbox.addr + MCP_SYSBOX_OFF_CMD);

  /* FIXME: remove the spinlock and check that the callers are serialized */
  mx_spin_lock(&is->cmd.spinlock);

  is->cmd.kreq.cmd.req.index = htonl(index);
  is->cmd.kreq.cmd.req.data0 = htonl(data0);
  is->cmd.kreq.cmd.req.data1 = htonl(data1);
  is->cmd.kreq.cmd.req.cmd = cmd;
  is->cmd.kreq.cmd.req.type = MCP_KREQ_COMMAND;

  done->status = -1;

  mx_spin_lock_irqsave(&is->kreqq_spinlock, flags);
  is->board_ops.write_kreq(is, &is->cmd.kreq);
  mx_spin_unlock_irqrestore(&is->kreqq_spinlock, flags);
  
  /* wait for completion (commands should complete quickly, not worth 
     sleeping over it) */
  sleep = MCP_COMMAND_TIMEOUT * 1000;
  while ((done->status == -1) && (sleep > 0)) {
    sleep -= 10;
    mx_spin(10);
    MAL_READBAR();
  }

  if (done->status == -1) {
    /* command timed out */
    if (mx_is_dead(is)) {
      status = -EIO;
      goto abort_with_lock;
    }
    MX_WARN(("command %d timed out, marking instance %d dead\n", cmd, is->id));
    mx_mark_board_dead(is, MX_DEAD_COMMAND_TIMEOUT, cmd);
  }
  if (mx_is_dead(is)) {
    status = -EIO;
    goto abort_with_lock;
  }
  
  *result = ntohl(done->result);
  
  switch (ntohl(done->status)) {
  case MCP_CMD_OK:
    status = 0;
    break;
    
  case MCP_CMD_ERROR_RANGE:
    status = -ERANGE;
    break;
    
  case MCP_CMD_ERROR_BUSY:
    status = -EBUSY;
    break;
    
  case MCP_CMD_ERROR_CLOSED:
    status = -EBADF;
    break;

  case MCP_CMD_UNKNOWN:
    status = -ENOSYS;
    break;
    
  default:
    status = -ENXIO;
    break;
  }

 abort_with_lock:
  mx_spin_unlock(&is->cmd.spinlock);

  return status;
}
