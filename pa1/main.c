#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "child.h"
#include "ipcio.h"

void spawn_children(uint8_t num_children) {
  IPCIO ipcio = {.id = PARENT_ID, .num_children = num_children};
  // todo: check retval != -1
  ipc_init(&ipcio);

  for (uint8_t i = 0; i < num_children; ++i) {
    // todo: check retval != -1
    if (fork() == 0) {
      // inside child process
      ipcio.id = PARENT_ID + 1 + i;
      child_start(ipcio);
      return;
    }
  }

  for (uint8_t i = 0; i < num_children; ++i)
    wait(NULL);
}

int main(int argc, char** argv) {
  uint8_t num_children = 0;

  char opt;
  while ((opt = getopt(argc, argv, "p:")) != -1)
    if (opt == 'p')
      num_children = strtoul(optarg, NULL, 10);

  if (num_children < 1 || num_children > MAX_CHILDREN) {
    fprintf(stderr, "Usage: %s -p num_children (1 <= num_children <= %d)\n",
            argv[0], MAX_CHILDREN);
    exit(1);
  }

  spawn_children(num_children);
}
