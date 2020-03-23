#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "child.h"
#include "ipcext.h"
#include "log.h"

void spawn_children(local_id num_children) {
  IPCIO* ipcio = ipc_init_parent(num_children);
  if (ipcio == NULL)
    log_panic(PARENT_ID, "failed to initialize IPC state");

  Message buf;

  log_init_pipes_log();
  ipc_iterate_pipes(ipcio, log_pipe);
  log_close_pipes_log();

  for (local_id i = 0; i < num_children; ++i) {
    // todo: check retval != -1
    if (fork() == 0) {
      // inside child process
      ipc_init_child(ipcio, i);
      child_entry(ipcio, &buf);
      return;
    }
  }

  log_init_events_log();
  ipc_ext_await_all_started(ipcio, &buf);
  ipc_ext_await_all_done(ipcio, &buf);

  for (local_id i = 0; i < num_children; ++i)
    wait(NULL);
}

int main(int argc, char** argv) {
  local_id num_children = 0;

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
