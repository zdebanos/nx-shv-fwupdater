#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "shv_tree.h"
#include "shv_file_com.h"
#include "shv_methods.h"
#include "ulut/ul_utdefs.h"

#include "appl_shv.h"

#define LINUX_TESTING

/****************************************************************************/

int shv_root_device_type(shv_con_ctx_t * shv_ctx, shv_node_t* item, int rid);

// fwUpdate methods
int shv_file_crc(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid);
int shv_file_write(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid);
int shv_file_stat(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid);
int shv_file_size(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid);

// fwStable methods
int shv_file_confirmed(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid);

// .device methods
int shv_device_reset(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid);

const shv_method_des_t shv_dev_root_dmap_item_device_type =
{
  .name = "deviceType",
  .method = shv_root_device_type
};

const shv_method_des_t shv_dev_fwUpdate_dmap_item_crc =
{
  .name = "crc",
  .method = shv_file_crc
};

const shv_method_des_t shv_dev_fwUpdate_dmap_item_write =
{
  .name = "write",
  .method = shv_file_write
};

const shv_method_des_t shv_dev_fwUpdate_dmap_item_stat =
{
  .name = "stat",
  .method = shv_file_stat
};

const shv_method_des_t shv_dev_fwUpdate_dmap_item_size =
{
  .name = "size",
  .method = shv_file_size
};

const shv_method_des_t shv_dev_fwStable_dmap_item_confirmed =
{
  .name = "confirm",
  .method = shv_file_confirmed
};

const shv_method_des_t shv_dev_dotdevice_dmap_item_reset =
{
  .name = "reset",
  .method = shv_device_reset
};

const shv_method_des_t * const shv_dev_fwUpdate_dmap_items[] =
{
  &shv_dev_fwUpdate_dmap_item_crc,
  &shv_dmap_item_dir,
  &shv_dmap_item_ls,
  &shv_dev_fwUpdate_dmap_item_size,
  &shv_dev_fwUpdate_dmap_item_stat,
  &shv_dev_fwUpdate_dmap_item_write,
};

const shv_method_des_t * const shv_dev_fwStable_dmap_items[] =
{
  &shv_dev_fwStable_dmap_item_confirmed,
  &shv_dmap_item_dir,
  &shv_dmap_item_ls,
};

const shv_method_des_t * const shv_dev_dotdevice_dmap_items[] =
{
  &shv_dmap_item_dir,
  &shv_dmap_item_ls,
  &shv_dev_dotdevice_dmap_item_reset,
};

const shv_method_des_t * const shv_dev_root_dmap_items[] =
{
  &shv_dev_root_dmap_item_device_type,
  &shv_dmap_item_dir,
  &shv_dmap_item_ls,
};

const shv_dmap_t shv_dev_root_dmap =
{
  .methods = 
  {
    .items = (void **)shv_dev_root_dmap_items,
    .count = 3,
    .alloc_count = 0,
  }
};

const shv_dmap_t shv_dev_fwUpdate_dmap =
{
  .methods =
  {
    .items = (void **)shv_dev_fwUpdate_dmap_items,
    .count = 6,
    .alloc_count = 0,
  }
};

const shv_dmap_t shv_dev_fwStable_dmap =
{
  .methods =
  {
    .items = (void **)shv_dev_fwStable_dmap_items,
    .count = 3,
    .alloc_count = 0,
  }
};

const shv_dmap_t shv_dev_dotdevice_dmap =
{
  .methods =
  {
    .items = (void **)shv_dev_dotdevice_dmap_items,
    .count = 3,
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

int shv_root_device_type(shv_con_ctx_t * shv_ctx, shv_node_t *item, int rid)
{
  const char *str = "NuttX Device";
  shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
  shv_send_str(shv_ctx, rid, str);
  return 0;
}

int shv_file_crc(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
  shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
  shv_send_int(shv_ctx, rid, 0);
  return 0;
}

int shv_file_write(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
  int ret;
  printf("Write called!\n");
  ret = shv_process_write(shv_ctx, rid, (shv_file_node_t *) item);
  shv_confirm_write(shv_ctx, rid, (shv_file_node_t *) item);
  return 0;
}

int shv_file_stat(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
  printf("Stat called!\n");
  shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
  shv_send_stat(shv_ctx, rid, (shv_file_node_t *) item);
  return 0;
}

int shv_file_size(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
  shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
  shv_send_size(shv_ctx, rid, (shv_file_node_t *) item);
  return 0;
}

int shv_file_confirmed(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
  printf("Confirmed called!\n");
  shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
  shv_send_int(shv_ctx, rid, 0);
  return 0;
}

int shv_device_reset(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
  printf("Reset called!\n");
  shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
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
  shv_node_t *tree_root, *dotdevice_node, *fwStable_node;
  shv_file_node_t *fwUpdate_node;
  shv_node_typed_val_t *item_val;

  int mode = 0;

  tree_root = shv_tree_node_new("", &shv_dev_root_dmap, 0);
  if (tree_root == NULL) {
    fprintf(stderr, "ERROR: shv_tree_node_new failed\n");
    return NULL;
  }
  
  // create new nodes and append them to the root
  fwUpdate_node = shv_tree_file_node_new("fwUpdate", &shv_dev_fwUpdate_dmap, 0);
  if (fwUpdate_node == NULL)  {
    fprintf(stderr, "ERROR: shv_tree_node_new failed\n");
    free(tree_root);
    return NULL;
  }
  // before adding the node to the tree, initialize its parameters 
  int flags = O_CREAT | O_RDWR;
  fwUpdate_node->fd = open("shvfile", flags);
  if (fwUpdate_node->fd < 0) {
    perror("open");
    free(tree_root);
    free(fwUpdate_node);
    return NULL;
  }
  fwUpdate_node->file_type = REGULAR;
  fwUpdate_node->crc = -1;

  shv_tree_add_child(tree_root, (shv_node_t*) fwUpdate_node);
  
  
  fwStable_node = shv_tree_node_new("fwStable", &shv_dev_fwStable_dmap, 0);
  if (fwStable_node == NULL) {
    fprintf(stderr, "ERROR: shv_tree_node_new failed\n");
    free(fwUpdate_node);
    free(tree_root);
    return NULL;
  }
  shv_tree_add_child(tree_root, fwStable_node);
  
  dotdevice_node = shv_tree_node_new(".device", &shv_dev_dotdevice_dmap, 0);
  if (dotdevice_node == NULL) {
    fprintf(stderr, "ERROR: shv_tree_node_new failed\n");
    free(fwUpdate_node);
    free(fwStable_node);
    free(tree_root);
    return NULL;
  }
  shv_tree_add_child(tree_root, dotdevice_node);
  
  return tree_root;
}

/****************************************************************************
 * Name: shv_file_tree_init
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

