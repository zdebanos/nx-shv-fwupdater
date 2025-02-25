#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "shv_tree.h"
#include "shv_file_com.h"
#include "shv_methods.h"
#include "ulut/ul_utdefs.h"

#include "appl_shv.h"

#define LINUX_TESTING

enum file_const_e
{
  FILE_TYPE     = 0,
  FILE_SIZE     = 0x2000,
  FILE_PAGESIZE = 4096
};

/****************************************************************************/

int shv_device_type(shv_con_ctx_t * shv_ctx, shv_node_t* item, int rid);
int shv_file_crc(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid);
int shv_file_write(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid);
int shv_file_stat(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid);
int shv_file_size(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid);

const shv_method_des_t shv_dev_root_dmap_item_device_type =
{
  .name = "deviceType", .method = shv_device_type
};

const shv_method_des_t shv_dev_root_dmap_item_crc =
{
  .name = "crc",
  .method = shv_file_crc
};

const shv_method_des_t shv_dev_root_dmap_item_write =
{
  .name = "write",
  .method = shv_file_write
};

const shv_method_des_t shv_dev_root_dmap_item_stat =
{
  .name = "stat",
  .method = shv_file_stat
};

const shv_method_des_t shv_dev_root_dmap_item_size =
{
  .name = "size",
  .method = shv_file_size
};

const shv_method_des_t * const shv_dev_root_dmap_items[] =
{
  &shv_dev_root_dmap_item_crc,
  &shv_dev_root_dmap_item_device_type,
  &shv_dmap_item_dir,
  &shv_dmap_item_ls,
  &shv_dev_root_dmap_item_size,
  &shv_dev_root_dmap_item_stat,
  &shv_dev_root_dmap_item_write,
};

const shv_dmap_t shv_dev_root_dmap =
{
  .methods = 
  {
    .items = (void **)shv_dev_root_dmap_items,
    .count = 7,
    .alloc_count = 0,
  }
};

static int shv_file_fd;

/****************************************************************************
 * Name: shv_device_type
 *
 * Description:
 *   Method "deviceType".
 *
 ****************************************************************************/

int shv_device_type(shv_con_ctx_t * shv_ctx, shv_node_t* item, int rid)
{
  const char *str = "nuttx-fwupdater";

  shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);

  shv_send_str(shv_ctx, rid, str);

  return 0;
}

int shv_file_crc(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
  printf("Crc called!\n");
  shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
  shv_send_int(shv_ctx, rid, 0);
  return 0;
}

int shv_file_write(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
  char b[20];
  int off;
  printf("write called!\n");
  shv_process_write(&shv_ctx->unpack_ctx, b, sizeof(b), &off);
  
  //if (lseek(shv_file_fd, off, SEEK_SET) == (off_t)-1) {
  //  perror("lseek");
  //  return -1;
  //}

  shv_confirm_write(shv_ctx, rid);
  return 0;
}

int shv_file_stat(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
  printf("stat called!\n");
  shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
  shv_send_stat(shv_ctx, rid, FILE_TYPE, FILE_SIZE, FILE_PAGESIZE);
  return 0;
}

int shv_file_size(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
  printf("size called!\n");
  shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
  shv_send_size(shv_ctx, rid, FILE_SIZE);
  return 0;
}

/****************************************************************************
 * Name: shv_tree_create
 *
 * Description:
 *  Initialize and fill the SHV tree with blocks and parameters.
 *
 ****************************************************************************/

shv_node_t *shv_tree_create(void)
{
  shv_node_t *tree_root;
  shv_node_t *item;
  shv_node_t *item_parent;
  shv_node_typed_val_t *item_val;

  int mode = 0;

  tree_root = shv_tree_node_new("", &shv_dev_root_dmap, 0);
  if (tree_root == NULL) {
    fprintf(stderr, "ERROR: shv_tree_node_new failed\n");
    return NULL;
  }

  return tree_root;
}

/****************************************************************************
 * Name: shv_tree_init
 *
 * Description:
 *  Entry point for SHV related operations. Calls shv_tree_create to create
 *  a SHV tree and then initialize SHV connection.
 *
 ****************************************************************************/

shv_con_ctx_t *shv_tree_init(int fd)
{
  shv_node_t *tree_root;
  
  if (fd < 0) {
    return NULL;
  }
  shv_file_fd = fd;

  tree_root = shv_tree_create();
  if (tree_root == NULL) {
    fprintf(stderr, "ERROR: shv_tree_create() failed.\n");
    return NULL;
  }

  /* Initialize SHV connection */

  shv_con_ctx_t *ctx = shv_com_init(tree_root);
  if (ctx == NULL) {
    fprintf(stderr, "ERROR: shv_init() failed.\n");
    return NULL;
  }

  return ctx;
}

