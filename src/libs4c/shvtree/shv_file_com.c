/*
  COPYRIGHT (C) 2025 Stepan Pressl 
    <pressl.stepan@gmail.com>
    <pressste@fel.cvut.cz>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
*/

#include <stdio.h>
#include <cchainpack.h>
#include <ccpon.h>

#include "ccpcp.h"
#include "shv_tree.h"
#include "shv_file_com.h"

enum write_parser_state
{
  IMAP_START,
  REQUEST_1,
  LIST_START,
  OFFSET,
  BLOB,
  LIST_STOP,
  IMAP_STOP
};


void shv_send_stat(shv_con_ctx_t *shv_ctx, int rid, int file_type, int file_size, int page_size)
{
  // should be REGULAR, only!
  if (file_type != REGULAR) {
    return; 
  }

  ccpcp_pack_context_init(&shv_ctx->pack_ctx,shv_ctx->shv_data,
                          SHV_BUF_LEN, shv_overflow_handler);
  
  for (shv_ctx->shv_send = 0; shv_ctx->shv_send < 2; shv_ctx->shv_send++) {
    if (shv_ctx->shv_send) {
      cchainpack_pack_uint_data(&shv_ctx->pack_ctx, shv_ctx->shv_len);
    }
    
    shv_ctx->shv_len = 0;
    cchainpack_pack_uint_data(&shv_ctx->pack_ctx, 1);
    
    shv_pack_head_reply(shv_ctx, rid);

    // An IMap in a IMap.
    cchainpack_pack_imap_begin(&shv_ctx->pack_ctx);
    // Reply
    cchainpack_pack_int(&shv_ctx->pack_ctx, 2);
    cchainpack_pack_imap_begin(&shv_ctx->pack_ctx);

    // The first key (file type)
    cchainpack_pack_int(&shv_ctx->pack_ctx, FN_TYPE);
    cchainpack_pack_int(&shv_ctx->pack_ctx, file_type);
    
    // The second key (file size)
    cchainpack_pack_int(&shv_ctx->pack_ctx, FN_SIZE);
    cchainpack_pack_int(&shv_ctx->pack_ctx, file_size);

    // The third key (page size)
    cchainpack_pack_int(&shv_ctx->pack_ctx, FN_PAGESIZE);
    cchainpack_pack_int(&shv_ctx->pack_ctx, page_size);
    
    cchainpack_pack_container_end(&shv_ctx->pack_ctx);
    cchainpack_pack_container_end(&shv_ctx->pack_ctx);
    shv_overflow_handler(&shv_ctx->pack_ctx, 0); 
  }
}

void shv_send_size(shv_con_ctx_t *shv_ctx, int rid, int file_size)
{
  ccpcp_pack_context_init(&shv_ctx->pack_ctx,shv_ctx->shv_data,
                          SHV_BUF_LEN, shv_overflow_handler);
  
  for (shv_ctx->shv_send = 0; shv_ctx->shv_send < 2; shv_ctx->shv_send++) {
    if (shv_ctx->shv_send) {
      cchainpack_pack_uint_data(&shv_ctx->pack_ctx, shv_ctx->shv_len);
    }
    
    shv_ctx->shv_len = 0;
    cchainpack_pack_uint_data(&shv_ctx->pack_ctx, 1);
    
    shv_pack_head_reply(shv_ctx, rid);

    cchainpack_pack_imap_begin(&shv_ctx->pack_ctx);

    // Reply
    cchainpack_pack_int(&shv_ctx->pack_ctx, 2);
    cchainpack_pack_int(&shv_ctx->pack_ctx, file_size);
    
    cchainpack_pack_container_end(&shv_ctx->pack_ctx);
    shv_overflow_handler(&shv_ctx->pack_ctx, 0); 
  }
}

void shv_send_crc(shv_con_ctx_t *shv_ctx, int rid, unsigned int crc)
{
  ccpcp_pack_context_init(&shv_ctx->pack_ctx,shv_ctx->shv_data,
                          SHV_BUF_LEN, shv_overflow_handler);
  
  for (shv_ctx->shv_send = 0; shv_ctx->shv_send < 2; shv_ctx->shv_send++) {
    if (shv_ctx->shv_send) {
      cchainpack_pack_uint_data(&shv_ctx->pack_ctx, shv_ctx->shv_len);
    }
    
    shv_ctx->shv_len = 0;
    cchainpack_pack_uint_data(&shv_ctx->pack_ctx, 1);
    
    shv_pack_head_reply(shv_ctx, rid);

    cchainpack_pack_imap_begin(&shv_ctx->pack_ctx);

    // Reply
    cchainpack_pack_int(&shv_ctx->pack_ctx, 2);
    cchainpack_pack_int(&shv_ctx->pack_ctx, crc);
    
    cchainpack_pack_container_end(&shv_ctx->pack_ctx);
    shv_overflow_handler(&shv_ctx->pack_ctx, 0); 
  }
}

int shv_process_write(ccpcp_unpack_context *ctx, void *buf, int buf_size, int *offset)
{
  int ret;
  int state = IMAP_START;

  do {
    cchainpack_unpack_next(ctx);
    if (ctx->err_no != CCPCP_RC_OK) {
      return -1;
    }
    
    switch (state) {
    case IMAP_START: {
      // the start of imap, proceed next
      if (ctx->item.type == CCPCP_ITEM_IMAP) {
        state = LIST_START;
      }
      break;
    }
    case REQUEST_1: {
      // wait for UInt or Int (namely number 1)
      if (ctx->item.type == CCPCP_ITEM_INT) {
        if (ctx->item.as.Int == 1) {
          state = LIST_START;
        } else {
          // something different received
          // shv_unpack_skip()
          ctx->err_no = CCPCP_RC_MALFORMED_INPUT;
          state = IMAP_START;
        }
      } else if (ctx->item.type == CCPCP_ITEM_UINT) {
        if (ctx->item.as.UInt == 1) {
          state = LIST_START;
        } else {
          // something different received
          ctx->err_no = CCPCP_RC_MALFORMED_INPUT;
        }
      } else {
        // Int or UInt expected!
        ctx->err_no = CCPCP_RC_MALFORMED_INPUT;
      }
    }
    case LIST_START: {
      if (ctx->item.type == CCPCP_ITEM_LIST) {
        state = OFFSET;
      } else {
        // shv_unpack_skip
        ctx->err_no = CCPCP_RC_MALFORMED_INPUT;
      }
    }
    case OFFSET: {
      if (ctx->item.type == CCPCP_ITEM_INT) {
        *offset = ctx->item.as.Int;
        state = BLOB;
      } else { 
        ctx->err_no = CCPCP_RC_MALFORMED_INPUT;
      }
    }
    case BLOB: {
      if (ctx->item.type == CCPCP_ITEM_BLOB) {
        // this actually unpacks the blob!
        cchainpack_unpack_next(ctx);
        if (ctx->err_no == CCPCP_RC_OK) {
          state = LIST_STOP;
        }
      }
    }
    case LIST_STOP: {
      if (ctx->item.type == CCPCP_ITEM_CONTAINER_END) {
        state = IMAP_STOP;
      } else {
        ctx->err_no = CCPCP_RC_MALFORMED_INPUT;
      }
    }
    case IMAP_STOP: {
      if (ctx->item.type != CCPCP_ITEM_CONTAINER_END) {
        // yikes
        ctx->err_no = CCPCP_RC_MALFORMED_INPUT;
      }
    }
    default:
      break;
    }
  } while (ctx->err_no == CCPCP_RC_OK);

  ret = -ctx->err_no;
  ctx->err_no = CCPCP_RC_OK;
  return ret; 
}

void shv_confirm_write(shv_con_ctx_t *shv_ctx, int rid)
{
  for (shv_ctx->shv_send = 0; shv_ctx->shv_send < 2; shv_ctx->shv_send++) {
    if (shv_ctx->shv_send) {
      cchainpack_pack_uint_data(&shv_ctx->pack_ctx, shv_ctx->shv_len);
    }
    
    shv_ctx->shv_len = 0;
    cchainpack_pack_uint_data(&shv_ctx->pack_ctx, 1);
    
    shv_pack_head_reply(shv_ctx, rid);

    // Empty IMap
    cchainpack_pack_imap_begin(&shv_ctx->pack_ctx);
    cchainpack_pack_container_end(&shv_ctx->pack_ctx);

    shv_overflow_handler(&shv_ctx->pack_ctx, 0); 
  }
}