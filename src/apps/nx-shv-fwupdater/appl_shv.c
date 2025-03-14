#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "shv_tree.h"
#include "shv_file_com.h"
#include "shv_methods.h"
#include "ulut/ul_utdefs.h"

#include "appl_shv.h"

#if defined(__linux__)

#define LINUX_TESTING
#define LINUX_TESTING_FILE_SIZE (16 * 1024 * 1024)
#define LINUX_TESTING_PAGE_SIZE (4 * 1024)
#define LINUX_TESTING_FILE_NAME "fwupdate.img"

inline static
uint32_t crc32part(const uint8_t *src, size_t len, uint32_t crc32val)
{
  return 0;
}

#else /*__linux__*/

#include <sys/boardctl.h>
#include <nxboot.h>
#include <nuttx/mtd/mtd.h>
#include <nuttx/crc32.h>

#endif /*__linux__*/

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

static bool nofirstwrite = false;
static uint8_t crcbuf[256];

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

static uint32_t shv_file_calculate_crc(int size, int offset, int fd)
{
  uint32_t crc = 0;

  // first, move to the desired offset
  if (lseek(fd, offset, SEEK_SET) == (off_t) -1) {
    perror("lseek");
    return 0;
  }

  while (size > 0) {
    int to_read, readsize;
    if (size >= sizeof(crcbuf)) {
      to_read = sizeof(crcbuf);
    } else {
      to_read = size;
    }
    readsize = read(fd, crcbuf, to_read);
    if (readsize < 0) {
      perror("read in crc");
      return 0;
    }
    size -= readsize;
    crc = crc32part(crcbuf, readsize, crc);
  }

  return crc;
}

int shv_file_crc(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
  int ret;
  int offset, size;
  uint32_t crc;
  shv_file_node_t *file = (shv_file_node_t *) item;

#ifndef LINUX_TESTING
  ioctl(file->fd, BIOC_FLUSH);
  // flush does not work properly, so we just do this hack:
  // first close the file and then reopen it, this actually flushes the data
  close(file->fd);
  file->fd = nxboot_open_update_partition();
#else /*LINUX_TESTING*/
  fsync(file->fd);
#endif /*LINUX_TESTING*/

  ret = shv_process_crc(shv_ctx, rid, file);
  if (ret == 0) {
    // this shall calculate the crc over the whole file!
    offset = 0;
    size = file->file_size;
  } else if (ret == 1) {
    // this shall calculate the crc from offset to end
    offset = file->crc_offset;
    size = file->file_size - file->crc_offset;
  } else if (ret == 2) {
    // this shall calculate the crc within a specified range
    offset = file->crc_offset;
    size = file->crc_size;
  } else {
    ret = -1;
  }

  file->received_bytes = 0;
  if (ret >= 0) {
    crc = shv_file_calculate_crc(size, offset, file->fd);
  } else {
    crc = 0;
  }
  shv_send_crc(shv_ctx, rid, (shv_file_node_t *) item, crc);
  return 0;
}

int shv_file_write(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
  int ret;
  shv_file_node_t *file = (shv_file_node_t *) item;

  if (!nofirstwrite) {
    nofirstwrite = true;
  }
  ret = shv_process_write(shv_ctx, rid, (shv_file_node_t *) item);
  if (ret < 0) {
    shv_send_error(shv_ctx, rid, "File write failed.");
  } else {
    shv_confirm_write(shv_ctx, rid, (shv_file_node_t *) item);
  }
  return 0;
}

int shv_file_stat(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
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
  shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
 #ifndef LINUX_TESTING
  if (nxboot_confirm() < 0) {
    shv_send_error(shv_ctx, rid, "Failed to confirm the image.");
  } else {
    shv_send_int(shv_ctx, rid, 0);
  }
 #else /*LINUX_TESTING*/
  shv_send_int(shv_ctx, rid, 0);
 #endif /*LINUX_TESTING*/
  return 0;
}

int shv_device_reset(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
  shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
  shv_send_int(shv_ctx, rid, 0);

  // wait a bit so the response arrives, then reset
  usleep(2000 * 1000);
 #ifndef LINUX_TESTING
  boardctl(BOARDIOC_RESET, BOARDIOC_RESETCAUSE_CPU_SOFT);
 #else /*LINUX_TESTING*/
  printf("Device reset requested!\n");
  exit(0);
 #endif /*LINUX_TESTING*/

  // should not get here
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
 #ifndef LINUX_TESTING
  struct mtd_geometry_s geometry;
 #else /*LINUX_TESTING*/
  const char *file_name;
 #endif /*LINUX_TESTING*/

  shv_node_t *tree_root, *dotdevice_node, *fwStable_node;
  shv_file_node_t *fwUpdate_node;

  // also, if we got here, we can confirm the previous image is OK
  printf("Version 36\n");

  tree_root = shv_tree_node_new("", &shv_dev_root_dmap, 0);
  if (tree_root == NULL) {
    fprintf(stderr, "ERROR: shv_tree_node_new failed\n");
    return NULL;
  }

  // create new nodes and append them to the root
  fwUpdate_node = shv_tree_file_node_new("fwUpdate", &shv_dev_fwUpdate_dmap, 0);
  if (fwUpdate_node == NULL)  {
    fprintf(stderr, "ERROR: shv_tree_node_new failed\n");
    goto err1;
  }

 #ifndef LINUX_TESTING
  fwUpdate_node->fd = nxboot_open_update_partition();
 #else /*LINUX_TESTING*/
  file_name = LINUX_TESTING_FILE_NAME;
  printf("Opening %s\n", file_name);
  fwUpdate_node->fd = open(file_name, O_RDWR);
 #endif /*LINUX_TESTING*/

  if (fwUpdate_node->fd < 0) {
    perror("open");
    goto err2;
  }

 #ifndef LINUX_TESTING
  if (ioctl(fwUpdate_node->fd, MTDIOC_GEOMETRY, (unsigned long)((uintptr_t)&geometry)) < 0) {
    perror("ioctl");
    goto err3;
  }
  fwUpdate_node->file_size = geometry.erasesize * geometry.neraseblocks;
  fwUpdate_node->file_pagesize = geometry.blocksize;
 #else /*LINUX_TESTING*/
  fwUpdate_node->file_size = LINUX_TESTING_FILE_SIZE;
  fwUpdate_node->file_pagesize = LINUX_TESTING_PAGE_SIZE;
 #endif /*LINUX_TESTING*/

  fwUpdate_node->file_type = REGULAR;
  fwUpdate_node->file_offset = 0;
  fwUpdate_node->received_bytes = 0;
  fwUpdate_node->crcstate = C_IMAP_START;

  shv_tree_add_child(tree_root, (shv_node_t*) fwUpdate_node);

  fwStable_node = shv_tree_node_new("fwStable", &shv_dev_fwStable_dmap, 0);
  if (fwStable_node == NULL) {
    fprintf(stderr, "ERROR: shv_tree_node_new failed\n");
    goto err3;
  }
  shv_tree_add_child(tree_root, fwStable_node);

  dotdevice_node = shv_tree_node_new(".device", &shv_dev_dotdevice_dmap, 0);
  if (dotdevice_node == NULL) {
    fprintf(stderr, "ERROR: shv_tree_node_new failed\n");
    goto err4;
  }
  shv_tree_add_child(tree_root, dotdevice_node);

  return tree_root;

err4:
  free(fwStable_node);
err3:
  close(fwUpdate_node->fd);
err2:
  free(fwUpdate_node);
err1:
  free(tree_root);
  return NULL;
}

/****************************************************************************
 * Name: shv_file_tree_init
 *
 * Description:
 *  Entry point for SHV related operations. Calls shv_tree_create to create
 *  a SHV tree and then initialize SHV connection.
 *
 ****************************************************************************/

shv_con_ctx_t *shv_tree_init(void)
{
  shv_node_t *tree_root;

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

