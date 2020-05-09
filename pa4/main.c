#define _POSIX_C_SOURCE 200809L
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "child.h"
#include "log.h"
#include "parent.h"

void spawn_children(local_id num_children, bool mutexl) {
  IPCIO* ipcio = ipc_init(num_children);
  if (ipcio == NULL)
    log_panic(PARENT_ID, "failed to initialize IPC state");

  ipc_use_nonblocking_io(ipcio);

  Message buf;

  log_init_events_log();
  log_init_pipes_log();
  ipc_iterate_pipes(ipcio, log_pipe);
  log_close_pipes_log();

  for (local_id i = 0; i < num_children; ++i) {
    int fork_res = fork();
    if (fork_res == 0) {
      // inside child process
      ipc_set_up_child(ipcio, i);
      child_entry(ipcio, &buf, num_children, mutexl);
      return;
    } else if (fork_res == -1) {
      log_panic(PARENT_ID, "failed to create child process");
    }
  }

  ipc_set_up_parent(ipcio);
  parent_entry(ipcio, &buf);

  for (local_id i = 0; i < num_children; ++i)
    wait(NULL);
}

int main(int argc, char** argv) {
  local_id num_children = 0;

  bool mutexl = false;
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--mutexl") == 0) {
      mutexl = true;
    } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
      num_children = strtoul(argv[++i], NULL, 10);
    }
  }

  if (num_children < 1 || num_children > MAX_CHILDREN) {
    fprintf(stderr, "Usage: %s -p num_children (1 <= num_children <= %d)\n", argv[0], MAX_CHILDREN);
    exit(1);
  }

  spawn_children(num_children, mutexl);
}
