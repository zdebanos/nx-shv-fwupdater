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
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <cchainpack.h>
#include <ccpon.h>
#include <unistd.h>

#include "ccpcp.h"
#include "shv_com.h"
#include "shv_tree.h"
#include "shv_file_com.h"

void shv_send_stat(shv_con_ctx_t *shv_ctx, int rid, shv_file_node_t *item)
{
  // should be REGULAR, only!
  if (item->file_type != REGULAR) {
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
    cchainpack_pack_int(&shv_ctx->pack_ctx, item->file_type);
    
    // The second key (file size)
    cchainpack_pack_int(&shv_ctx->pack_ctx, FN_SIZE);
    cchainpack_pack_int(&shv_ctx->pack_ctx, item->file_size);

    // The third key (page size)
    cchainpack_pack_int(&shv_ctx->pack_ctx, FN_PAGESIZE);
    cchainpack_pack_int(&shv_ctx->pack_ctx, item->file_pagesize);
    
    // The fifth key (max send size)
    cchainpack_pack_int(&shv_ctx->pack_ctx, 5);
    // receive the blobs each 128 bytes
    cchainpack_pack_int(&shv_ctx->pack_ctx, item->file_pagesize);
    
    cchainpack_pack_container_end(&shv_ctx->pack_ctx);
    cchainpack_pack_container_end(&shv_ctx->pack_ctx);
    shv_overflow_handler(&shv_ctx->pack_ctx, 0); 
  }
}

void shv_send_size(shv_con_ctx_t *shv_ctx, int rid, shv_file_node_t *item)
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
    cchainpack_pack_int(&shv_ctx->pack_ctx, item->file_size);
    
    cchainpack_pack_container_end(&shv_ctx->pack_ctx);
    shv_overflow_handler(&shv_ctx->pack_ctx, 0); 
  }
}

void shv_send_crc(shv_con_ctx_t *shv_ctx, int rid, shv_file_node_t *item)
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
    cchainpack_pack_int(&shv_ctx->pack_ctx, item->crc);
    
    cchainpack_pack_container_end(&shv_ctx->pack_ctx);
    shv_overflow_handler(&shv_ctx->pack_ctx, 0); 
  }
}

int shv_process_write(shv_con_ctx_t *shv_ctx, int rid, shv_file_node_t *item)
{
  int ret;
  ccpcp_unpack_context *ctx = &shv_ctx->unpack_ctx;

  do {
    cchainpack_unpack_next(ctx);
    if (ctx->err_no != CCPCP_RC_OK) {
      return -1;
    }
    
    switch (item->state) {
    case IMAP_START: {
      // the start of imap, proceed next
      if (ctx->item.type == CCPCP_ITEM_IMAP) {
        item->state = REQUEST_1;
      }
      break;
    }
    case REQUEST_1: {
      // wait for UInt or Int (namely number 1)
      if (ctx->item.type == CCPCP_ITEM_INT) {
        if (ctx->item.as.Int == 1) {
          item->state = LIST_START;
        } else {
          // something different received
          shv_unpack_discard(shv_ctx);
          ctx->err_no = CCPCP_RC_LOGICAL_ERROR;
          item->state = IMAP_START;
        }
      } else if (ctx->item.type == CCPCP_ITEM_UINT) {
        if (ctx->item.as.UInt == 1) {
          item->state = LIST_START;
        } else {
          // something different received
          shv_unpack_discard(shv_ctx);
          ctx->err_no = CCPCP_RC_LOGICAL_ERROR;
          item->state = IMAP_START;
        }
      } else {
        // Int or UInt expected!
        shv_unpack_discard(shv_ctx);
        ctx->err_no = CCPCP_RC_LOGICAL_ERROR;
        item->state = IMAP_START;
      }
      break;
    }
    case LIST_START: {
      if (ctx->item.type == CCPCP_ITEM_LIST) {
        item->state = OFFSET;
      } else {
        shv_unpack_discard(shv_ctx);
        ctx->err_no = CCPCP_RC_LOGICAL_ERROR;
        item->state = IMAP_START;
      }
      break;
    }
    case OFFSET: {
      if (ctx->item.type == CCPCP_ITEM_INT) {
        // save the loaded offset into the struct
        item->file_offset = ctx->item.as.Int;
        item->state = BLOB;
        
        if (lseek(item->fd, item->file_offset, SEEK_SET) == (off_t)-1) {
          // how to handle errors?
          ctx->err_no = CCPCP_RC_LOGICAL_ERROR;
          item->state = IMAP_START;
        } else {
          printf("lseek ok %d\n", item->file_offset);
        }
        
      } else { 
        shv_unpack_discard(shv_ctx);
        ctx->err_no = CCPCP_RC_LOGICAL_ERROR;
        item->state = IMAP_START;
      }
      break;
    }
    case BLOB: {
      if (ctx->item.type == CCPCP_ITEM_BLOB) {
        // write to the fd
        if (write(item->fd, ctx->item.as.String.chunk_start, ctx->item.as.String.chunk_size) < 0) {
          perror("write");
        }
        if (ctx->item.as.String.last_chunk) {
          // it is the last loaded chunk, we can now proceed
          item->state = LIST_STOP;
        }
      } else {
        shv_unpack_discard(shv_ctx);
        ctx->err_no = CCPCP_RC_MALFORMED_INPUT;
        item->state = IMAP_START;
      }
      break;
    }
    case LIST_STOP: {
      if (ctx->item.type == CCPCP_ITEM_CONTAINER_END) {
        item->state = IMAP_STOP;
      } else {
        shv_unpack_discard(shv_ctx);
        ctx->err_no = CCPCP_RC_MALFORMED_INPUT;
        item->state = IMAP_START;
      }
      break;
    }
    case IMAP_STOP: {
      if (ctx->item.type != CCPCP_ITEM_CONTAINER_END) {
        // something horrible happened
        ctx->err_no = CCPCP_RC_MALFORMED_INPUT;
        item->state = IMAP_START;
      } else {
        // restore state and return
        printf("Process write completed succesfully!\n");
        item->state = IMAP_START;
        return 0;
      }
      break;
    }
    default:
      break;
    }
  } while (ctx->err_no == CCPCP_RC_OK);

  ret = -ctx->err_no;
  ctx->err_no = CCPCP_RC_OK;
  return ret; 
}

void shv_confirm_write(shv_con_ctx_t *shv_ctx, int rid, shv_file_node_t *item)
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
