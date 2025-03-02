#include <stdio.h>
#include <unistd.h>

#include "src/shv_tree.h"
#include "src/shv_methods.h"
#include "src/appl_shv.h"

int get_priority_for_com(void)
{
  return 0;
}

int main(int argc, char *argv[])
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
