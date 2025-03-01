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

#ifndef _SHV_FILE_COM_H
#define _SHV_FILE_COM_H

#include "shv_com.h"
#include "shv_tree.h"

enum shv_file_type
{
  REGULAR = 0,
  SHV_FILE_TYPE_COUNT
};

enum shv_file_node_keys
{
  FN_TYPE = 0,
  FN_SIZE,
  FN_PAGESIZE,
  FN_ACCESSTIME,
  FN_MODTIME,
  FN_MAXWRITE,
  SHV_FILE_NODE_KEYS_COUNT
};

void     shv_send_stat(shv_con_ctx_t *shv_ctx, int rid, shv_file_node_t *item);
void     shv_send_size(shv_con_ctx_t *shv_ctx, int rid, shv_file_node_t *item);
void      shv_send_crc(shv_con_ctx_t *shv_ctx, int rid, shv_file_node_t *item);
void shv_confirm_write(shv_con_ctx_t *shv_ctx, int rid, shv_file_node_t *item);
int  shv_process_write(shv_con_ctx_t *shv_ctx, int rid, shv_file_node_t *item);

#endif /* _SHV_FILE_COM_H */
