#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include <shv/tree/shv_tree.h>
#include <shv/tree/shv_methods.h>
#include <ulut/ul_utdefs.h>
#include "appl_shv.h"

#define PARAMS_COUNT 5
#define PARAM_SRV    0
#define PARAM_PORT   1
#define PARAM_USER   2
#define PARAM_PASSWD 3
#define PARAM_MOUNT  4

static const char *prog_params[PARAMS_COUNT] =
{
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};

int get_priority_for_com(void)
{
  return 0;
}

void print_usage(void)
{
  printf("NuttX Updater over the Silicon Heaven protocol\n");
  printf("usage: shv_fwupdater -s <shv_server> -p <shv_port> -u <user> -P <password> -m <mount>\n");
}

int main(int argc, char *argv[])
{
  int port;
  struct shv_connection conn;
  shv_con_ctx_t *ctx;
  shv_node_t *tree_root;

  struct timespec sleep_time = {100, 0};
  int arg_idx;

  for (arg_idx = 1; arg_idx < argc; arg_idx++) {
    if (argv[arg_idx][0] == '-' && (strnlen(argv[arg_idx], 2) == 2)) {
      int param_idx = -1;
      switch (argv[arg_idx][1]) {
        case 's':
          param_idx = PARAM_SRV;
          break; 
        case 'p':
          param_idx = PARAM_PORT;
          break;
        case 'u':
          param_idx = PARAM_USER;
          break;
        case 'P':
          param_idx = PARAM_PASSWD;
          break;
        case 'm':
          param_idx = PARAM_MOUNT;
          break;
        default:
          break;
      }
      if (param_idx != -1 && arg_idx != argc - 1) {
        prog_params[param_idx] = argv[arg_idx + 1];
      }
    }
  }
  
  /* Check if all parameters are supplied */
  for (int i = 0; i < PARAMS_COUNT; ++i) {
    if (prog_params[i] == NULL) {
      puts("Not all parameters were supplied!");
      return 1;
    }
  }
  
  /* Try to convert the port to a number */
  port = atoi(prog_params[PARAM_PORT]); 
  
  shv_connection_init(&connection, SHV_TLAYER_TCPIP);
  conn.broker_user     = prog_params[PARAM_USER];
  conn.broker_password = prog_params[PARAM_PASSWD];
  conn.broker_mount    = prog_params[PARAM_MOUNT];
  if (shv_connection_tcpip_init(&connection, prog_params[PARAM_SRV], port) > 0) {
    puts("Can't init shv_connection!");
    return 1;
  }
  
  tree_root = shv_tree_create();
  if (tree_root == NULL) {
    puts("Can't create the tree!");
    return 1;
  }

  ctx = shv_com_init(tree_root, &connection);
  if (ctx == NULL) {
    fprintf(stderr, "ERROR: shv_tree_init() failed.\n");
    exit(1);
  }

  while (1) {
    clock_nanosleep(CLOCK_MONOTONIC, 0, &sleep_time, NULL);
  }
  
  return 0;
}