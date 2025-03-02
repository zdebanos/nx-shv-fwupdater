#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include "shv_tree.h"
#include "shv_methods.h"
#include "appl_shv.h"


int get_priority_for_com(void)
{
  return 0;
}

int main(int argv, char *argc[])
{
  shv_con_ctx_t *ctx;
  int wstatus = 0;
  struct timespec sleep_time = {100, 0};

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