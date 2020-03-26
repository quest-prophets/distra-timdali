#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "banking.h"
#include "ipcext.h"
#include "log.h"
#include "pa2345.h"

void transfer(void* parent_data, local_id src, local_id dst, balance_t amount) {
  IPCIO* ipcio = (IPCIO*)parent_data;

  Message* buf = malloc(sizeof(Message));

  buf->s_header.s_magic = MESSAGE_MAGIC;
  buf->s_header.s_type = TRANSFER;
  buf->s_header.s_payload_len = sizeof(TransferOrder);
  buf->s_header.s_local_time = get_physical_time();
  *((TransferOrder*)buf->s_payload) =
      (TransferOrder){.s_src = src, .s_dst = dst, .s_amount = amount};

  send(ipcio, src, buf);
  receive_blocking(ipcio, dst, buf);
  assert(buf->s_header.s_type == ACK);

  free(buf);
}

void parent_fill_history_callback(void* hist_ptr, local_id from, Message* buf) {
  AllHistory* history = (AllHistory*)hist_ptr;
  assert(history->s_history_len == from - 1);
  history->s_history[from - 1] = *((BalanceHistory*)buf->s_payload);
  history->s_history_len++;
}

void parent_entry(IPCIO* ipcio, Message* buf, local_id num_children) {
  ipc_ext_await_all_started(ipcio, buf);

  bank_robbery(ipcio, num_children);

  ipc_ext_set_status(buf, STOP);
  if (send_multicast(ipcio, buf) != 0)
    log_panic(ipc_id(ipcio), "failed to broadcast the STOP message\n");

  ipc_ext_await_all_done(ipcio, buf);

  AllHistory history = {.s_history_len = 0};
  ipc_ext_receive_all(ipcio, buf, BALANCE_HISTORY, &history,
                      parent_fill_history_callback);
  print_history(&history);
}

void child_entry(IPCIO* ipcio, Message* buf, balance_t balance) {
  ipc_ext_set_payload(buf, STARTED, log_started_fmt, 0, ipc_id(ipcio), getpid(),
                      getppid(), balance);
  if (send_multicast(ipcio, buf) != 0)
    log_panic(ipc_id(ipcio), "failed to broadcast the STARTED message\n");
  log_event(buf->s_payload);

  BalanceHistory history = {.s_id = ipc_id(ipcio), .s_history_len = 1};
  history.s_history[0] = (BalanceState){
      .s_balance = balance, .s_time = 0, .s_balance_pending_in = 0};

  while (1) {
    if (receive_any(ipcio, buf) == 0) {
      // fill in the missing balance history entries
      timestamp_t recv_time = get_physical_time();
      for (timestamp_t t = history.s_history_len; t < recv_time; ++t) {
        history.s_history[t] = (BalanceState){
            .s_balance = balance, .s_time = t, .s_balance_pending_in = 0};
        history.s_history_len++;
      }

      if (buf->s_header.s_type == STOP)
        break;
      if (buf->s_header.s_type == TRANSFER) {
        TransferOrder* transfer = (TransferOrder*)buf->s_payload;

        if (transfer->s_src == ipc_id(ipcio)) {
          balance -= transfer->s_amount;

          send(ipcio, transfer->s_dst, buf);
        } else if (transfer->s_dst == ipc_id(ipcio)) {
          balance += transfer->s_amount;

          buf->s_header.s_type = ACK;
          buf->s_header.s_payload_len = 0;
          send(ipcio, PARENT_ID, buf);
        }
      }
    }
  }

  ipc_ext_set_payload(buf, DONE, log_received_all_done_fmt, 0, ipc_id(ipcio));
  if (send_multicast(ipcio, buf) != 0)
    log_panic(ipc_id(ipcio), "failed to broadcast the DONE message\n");
  log_event(buf->s_payload);

  ipc_ext_await_all_done(ipcio, buf);

  buf->s_header.s_type = BALANCE_HISTORY;
  buf->s_header.s_payload_len = sizeof(BalanceHistory);
  *((BalanceHistory*)buf->s_payload) = history;
  send(ipcio, PARENT_ID, buf);
}

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
