#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "child.h"
#include "log.h"
#include "parent.h"

void start(local_id num_children, balance_t* start_balances) {
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
      ipc_set_up_child(ipcio, i);
      child_entry(ipcio, &buf, start_balances[i]);
      return;
    } else if (fork_res == -1) {
      log_panic(PARENT_ID, "failed to create child process");
    }
  }

  ipc_set_up_parent(ipcio);
  parent_entry(ipcio, &buf, num_children);

  // wait for child processes to exit
  for (local_id i = 0; i < num_children; ++i)
    wait(NULL);
}

int main(int argc, char** argv) {
  local_id nchildren = 0;
  balance_t start_balances[MAX_PROCESS_ID + 1];

  char opt;
  while ((opt = getopt(argc, argv, "p:")) != -1)
    if (opt == 'p')
      nchildren = strtoul(optarg, NULL, 10);

  if (nchildren < 1 || nchildren > MAX_CHILDREN || optind + nchildren != argc) {
    fprintf(stderr,
            "Usage: %s -p N S...\n"
            "where: N    is the number of child processes (1..%d)\n"
            "       S... is a space-separated list of N integers denoting\n"
            "            initial balance for each child process\n",
            argv[0], MAX_CHILDREN);
    return 1;
  }

  for (local_id child = 0; child < nchildren; ++child)
    start_balances[child] = strtoul(argv[optind++], NULL, 10);

  start(nchildren, start_balances);
  return 0;
}
