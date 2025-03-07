#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/boardctl.h>

#include "shv_tree.h"
#include "shv_file_com.h"
#include "shv_methods.h"
#include "appl_shv.h"

#include <nxboot.h>
#include <nuttx/mtd/mtd.h>
#include <nuttx/crc32.h>

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

static int flash_partition_info(int fd, struct mtd_geometry_s *geometry);
static int flash_partition_erase_last_sector(int fd, struct mtd_geometry_s geometry);
static bool nofirstwrite = false;
uint8_t crcbuf[256];

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
    size -= sizeof(crcbuf);
    readsize = read(fd, crcbuf, to_read);
    crc = crc32part(crcbuf, to_read, crc);
  }

  return crc;
}

int shv_file_crc(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
  int ret;
  int flash_reads;
  int offset, size;
  uint32_t crc;
  struct mtd_geometry_s geometry;
  shv_file_node_t *file = (shv_file_node_t *) item;
  const char *file_name = NULL;

  ioctl(file->fd, BIOC_FLUSH);
  // flush does not work properly, so we just do this hack:
  // first close the file and then reopen it, this actually flushes the data
  close(file->fd);
  if (file->slotnum == NXBOOT_SECONDARY_SLOT_NUM) {
    file_name = CONFIG_NXBOOT_SECONDARY_SLOT_PATH;
  } else if (file->slotnum == NXBOOT_TERTIARY_SLOT_NUM) {
    file_name = CONFIG_NXBOOT_TERTIARY_SLOT_PATH;
  } else {
    return ERROR;
  }
  file->fd = open(file_name, O_RDWR);

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
    // during the first write, the flash_partition_area must be erased
    struct mtd_geometry_s geometry;
    if (flash_partition_info(file->fd, &geometry) >= 0) {
      if (flash_partition_erase_last_sector(file->fd, geometry) >= 0) {
        printf("First page erase!\n");
        nofirstwrite = true;
      }
    }
  } 
  ret = shv_process_write(shv_ctx, rid, (shv_file_node_t *) item);
  shv_confirm_write(shv_ctx, rid, (shv_file_node_t *) item);
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
  shv_send_int(shv_ctx, rid, 0);
  return 0;
}

int shv_device_reset(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
  shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
  shv_send_int(shv_ctx, rid, 0);

  // wait a bit so the response arrives, then reset
  usleep(2000 * 1000);
  boardctl(BOARDIOC_RESET, BOARDIOC_RESETCAUSE_CPU_SOFT);

  // should not get here
  return 0;
}

/****************************************************************************
 * Name: flash_partition_erase_last_sector
 *
 * Description:
 *   Erases the last sector of the partition
 *
 * Input parameters:
 *   fd: Valid file descriptor.
 *
 * Returned Value:
 *   0 on success, -1 on failure.
 *
 ****************************************************************************/

static int flash_partition_erase_last_sector(int fd, struct mtd_geometry_s geometry)
{
  int ret;
  struct mtd_erase_s erase;

  erase.startblock = geometry.neraseblocks - 1;
  erase.nblocks = 1;

  ret = ioctl(fd, MTDIOC_ERASESECTORS, &erase);
  if (ret < 0) {
    return ERROR;
  }

  return OK;
}

/****************************************************************************
 * Name: flash_partition_info
 *
 * Description:
 *   Get flash parameters
 *
 * Input parameters:
 *   fd: Valid file descriptor.
 *
 * Returned Value:
 *   0 on success, -1 on failure.
 *
 ****************************************************************************/

static int flash_partition_info(int fd, struct mtd_geometry_s *geometry)
{
  int ret;
  ret = ioctl(fd, MTDIOC_GEOMETRY, (unsigned long)((uintptr_t)geometry));
  if (ret < 0)
    {
      return ERROR;
    }
  return OK;
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
  const char *file_name;
  struct nxboot_state nxb_state;
  struct mtd_geometry_s geometry;
  
  shv_node_t *tree_root, *dotdevice_node, *fwStable_node;
  shv_file_node_t *fwUpdate_node;

  // also, if we got here, we can confirm the previous image is OK
  printf("Version 33\n");
  printf("Trying to confirm image\n");
  if (nxboot_confirm() < 0) {
    perror("nxboot confirm");
  } else {
    printf("Image confirm OK!\n");
  }

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

  // before adding the node to the tree, initialize its parameters 
  // this requires getting the information about the flash memory
  // and since this only now works with nxboot, we must get the
  // right partition 
  
  if (nxboot_get_state(&nxb_state) < 0) {
    perror("nxboot_get_state");
    goto err2;
  }
  
  // now, choose the right partition
  if (nxb_state.update == NXBOOT_SECONDARY_SLOT_NUM) {
    file_name = CONFIG_NXBOOT_SECONDARY_SLOT_PATH;
    fwUpdate_node->slotnum = NXBOOT_SECONDARY_SLOT_NUM;
  } else if (nxb_state.update == NXBOOT_TERTIARY_SLOT_NUM) {
    file_name = CONFIG_NXBOOT_TERTIARY_SLOT_PATH;
    fwUpdate_node->slotnum = NXBOOT_TERTIARY_SLOT_NUM;
  } else {
    fprintf(stderr, "Unexpected value in nxboot\n");
    goto err2;
  }
  printf("Opening %s\n", file_name);
  
  fwUpdate_node->fd = open(file_name, O_RDWR);
  if (fwUpdate_node->fd < 0) {
    perror("open");
    goto err2;
  }
  
  if (ioctl(fwUpdate_node->fd, MTDIOC_GEOMETRY, (unsigned long)((uintptr_t)&geometry)) < 0) {
    perror("ioctl");
    goto err3;
  }

  fwUpdate_node->file_type = REGULAR;
  fwUpdate_node->file_size = geometry.erasesize * geometry.neraseblocks;
  fwUpdate_node->file_offset = 0;
  fwUpdate_node->file_pagesize = geometry.blocksize;
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
  
  setenv("SHV_BROKER_IP", "147.32.87.165", 0);
  setenv("SHV_BROKER_PORT", "3755", 0);
  setenv("SHV_BROKER_USER", "mzapoknobs", 0);
  setenv("SHV_BROKER_PASSWORD", "d4268ee1bdb5605b4c", 0);
  setenv("SHV_BROKER_MOUNT", "test/SaMoCon-SHV", 0);
  
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

