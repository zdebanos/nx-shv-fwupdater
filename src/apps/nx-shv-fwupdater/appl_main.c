#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include "shv_tree.h"
#include "shv_methods.h"
#include "ulut/ul_utdefs.h"

#include "appl_shv.h"


int get_priority_for_com(void)
{
  return 0;
}

void print_usage(void)
{
  printf("NuttX Updater over silicon-heaven protokol\n");
  printf("usage: shv_fwupdater -s <shv_server> -p <shv_port> -u <user> -P <password> -m <mount>\n");
}

int main(int argc, char *argv[])
{
  shv_con_ctx_t *ctx;
  int wstatus = 0;
  struct timespec sleep_time = {100, 0};
  int arg_idx;

  for (arg_idx = 1; arg_idx < argc; arg_idx++) {
    const char *p = argv[arg_idx];
    const char *arg2env = NULL;
    if (*(p++) != '-')
       continue;
    switch (*(p++)) {
      case 'h':
         print_usage();
         return 0;
      case 's':
         arg2env = "SHV_BROKER_IP";
         break;
      case 'p':
         arg2env = "SHV_BROKER_PORT";
         break;
      case 'u':
         arg2env = "SHV_BROKER_USER";
         break;
      case 'P':
         arg2env = "SHV_BROKER_PASSWORD";
         break;
      case 'm':
         arg2env = "SHV_BROKER_MOUNT";
         break;
    }
    if (arg2env != NULL) {
      if (!*p)
         p = argv[++arg_idx];
      setenv(arg2env, p, 0);
    }
  }

  ctx = shv_tree_init();
  if (ctx == NULL) {
    fprintf(stderr, "ERROR: shv_tree_init() failed.\n");
    exit(1);
  }
  while (1) {
    //wait(&wstatus);
    clock_nanosleep(CLOCK_MONOTONIC, 0, &sleep_time, NULL);
  }

  return 0;
}