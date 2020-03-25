#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "banking.h"
#include "ipcext.h"
#include "log.h"

void transfer(void* parent_data, local_id src, local_id dst, balance_t amount) {
  // student, please implement me
}

void client_process_do_work(IPCIO* ipcio, Message* buf) {
  // bank_robbery(parent_data);
  // print_history(all);
}

void start(local_id num_children) {
  IPCIO* ipcio = ipc_init(num_children);
  if (ipcio == NULL)
    log_panic(PARENT_ID, "failed to initialize IPC state");

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
      // ???
      return;
    } else if (fork_res == -1) {
      log_panic(PARENT_ID, "failed to create child process");
    }
  }

  ipc_set_up_parent(ipcio);
  ipc_ext_await_all_started(ipcio, &buf);

  client_process_do_work(ipcio, &buf);

  ipc_ext_await_all_done(ipcio, &buf);

  for (local_id i = 0; i < num_children; ++i)
    wait(NULL);
}

int main(int argc, char** argv) {
  local_id nchildren = 0;

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

  start(nchildren);
  return 0;
}
