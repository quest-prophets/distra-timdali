#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "pa2345.h"
#include "banking.h"
#include "ipcext.h"
#include "log.h"

void transfer(void* parent_data, local_id src, local_id dst, balance_t amount) {
  IPCIO* ipcio = (IPCIO*)parent_data;

  // student, please implement me
}

void parent_entry(IPCIO* ipcio, Message* buf, local_id num_children) {
  ipc_ext_await_all_started(ipcio, buf);

  bank_robbery(ipcio, num_children + 1);

  ipc_ext_set_status(buf, STOP);
  if (send_multicast(ipcio, buf) != 0)
    log_panic(ipc_id(ipcio), "failed to broadcast the STOP message\n");

  ipc_ext_await_all_done(ipcio, buf);

  AllHistory history = {.s_history_len = num_children};
  // TODO: fill history with BALANCE_HISTORY from each child process

  print_history(&history);
}

void child_entry(IPCIO* ipcio, Message* buf, balance_t init_balance) {
  ipc_ext_set_payload(buf, STARTED, log_started_fmt, 0, ipc_id(ipcio), getpid(),
                      getppid(), init_balance);
  if (send_multicast(ipcio, buf) != 0)
    log_panic(ipc_id(ipcio), "failed to broadcast the STARTED message\n");
  log_event(buf->s_payload);

  while (1) {
    if (receive_any(ipcio, buf) == 0) {
      if (buf->s_header.s_type == STOP)
        break;
    }
  }

  ipc_ext_set_payload(buf, DONE, log_received_all_done_fmt, 0, ipc_id(ipcio));
  if (send_multicast(ipcio, buf) != 0)
    log_panic(ipc_id(ipcio), "failed to broadcast the DONE message\n");
  log_event(buf->s_payload);
}

void start(local_id num_children, balance_t *start_balances) {
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
