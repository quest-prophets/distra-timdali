#include "child.h"

#include <stdio.h>
#include <unistd.h>

#include "log.h"
#include "pa2345.h"
#include "time.h"

void record_history(BalanceHistory* history, BalanceState balance_state) {
  history->s_history[balance_state.s_time] = balance_state;
  history->s_history_len++;
}

void child_entry(IPCIO* ipcio, Message* buf, balance_t balance) {
  BalanceHistory history = {.s_id = ipc_id(ipcio), .s_history_len = 1};
  history.s_history[0] =
      (BalanceState){.s_balance = balance, .s_time = 0, .s_balance_pending_in = 0};

  child_sync_started(ipcio, buf, balance);

  while (1) {
    if (receive_any(ipcio, buf) == 0) {
      // fill in the missing balance history entries
      synchronize_time(buf->s_header.s_local_time);
      timestamp_t recv_t = get_lamport_time();
      for (timestamp_t t = history.s_history_len; t < recv_t; ++t)
        record_history(
            &history, (BalanceState){.s_balance = balance, .s_time = t, .s_balance_pending_in = 0});

      if (buf->s_header.s_type == STOP) {
        break;
      }
      if (buf->s_header.s_type == TRANSFER) {
        TransferOrder* transfer = (TransferOrder*)buf->s_payload;
        advance_lamport_time();
        buf->s_header.s_local_time = get_lamport_time();

        if (transfer->s_src == ipc_id(ipcio)) {
          balance -= transfer->s_amount;
          record_history(&history, (BalanceState){.s_balance = balance,
                                                  .s_time = recv_t,
                                                  .s_balance_pending_in = transfer->s_amount});
          log_eventf(log_transfer_out_fmt, get_lamport_time(), transfer->s_src, transfer->s_amount,
                     transfer->s_dst);

          send(ipcio, transfer->s_dst, buf);
        } else if (transfer->s_dst == ipc_id(ipcio)) {
          balance += transfer->s_amount;
          log_eventf(log_transfer_in_fmt, get_lamport_time(), transfer->s_dst, transfer->s_amount,
                     transfer->s_src);

          buf->s_header.s_type = ACK;
          buf->s_header.s_payload_len = 0;
          send(ipcio, PARENT_ID, buf);
        }
      }
    }
  }

  child_sync_done(ipcio, buf, balance);
  synchronize_time(buf->s_header.s_local_time);

  for (timestamp_t t = history.s_history_len; t < get_lamport_time(); ++t)
    record_history(&history,
                   (BalanceState){.s_balance = balance, .s_time = t, .s_balance_pending_in = 0});

  advance_lamport_time();
  ipc_ext_set_status(buf, BALANCE_HISTORY, get_lamport_time());
  buf->s_header.s_payload_len = sizeof(BalanceHistory);
  *((BalanceHistory*)buf->s_payload) = history;
  send(ipcio, PARENT_ID, buf);
}

void child_sync_started(IPCIO* ipcio, Message* buf, balance_t balance) {
  advance_lamport_time();
  timestamp_t time = get_lamport_time();
  ipc_ext_set_payload(buf, STARTED, time, log_started_fmt, time, ipc_id(ipcio), getpid(), getppid(),
                      balance);
  if (send_multicast(ipcio, buf) != 0)
    log_panic(ipc_id(ipcio), "failed to broadcast the STARTED message\n");
  log_event(buf->s_payload);

  ipc_ext_await_all_started(ipcio, buf);
}

void child_sync_done(IPCIO* ipcio, Message* buf, balance_t balance) {
  advance_lamport_time();
  timestamp_t time = get_lamport_time();
  ipc_ext_set_payload(buf, DONE, time, log_done_fmt, time, ipc_id(ipcio), balance);
  if (send_multicast(ipcio, buf) != 0)
    log_panic(ipc_id(ipcio), "failed to broadcast the DONE message\n");
  log_event(buf->s_payload);

  ipc_ext_await_all_done(ipcio, buf);
}
