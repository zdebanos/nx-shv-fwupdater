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

int main(int argv, char *argc[])
{
  shv_con_ctx_t *ctx;
  int wstatus = 0;
  struct timespec sleep_time = { 100, 0};

  
  /* Open the file descriptor */
  int flags = O_RDWR;
#ifdef LINUX_TESTING
  flags |= O_CREAT;
#endif
  int fd = open("shvfile", flags);
  if (fd < 0) {
    fprintf(stderr, "ERROR: openening file failed %d.\n", errno);
    exit(1);
  }
  
  ctx = shv_tree_init(fd);
  if (ctx == NULL) {
    close(fd);
    fprintf(stderr, "ERROR: shv_tree_init() failed.\n");
    exit(1);
  }
  while (1) {
    //wait(&wstatus);
    clock_nanosleep(CLOCK_MONOTONIC, 0, &sleep_time, NULL);
  }
  close(fd);

  return 0;
}