#include "child.h"

#include <unistd.h>

#include "log.h"
#include "pa2345.h"

void child_entry(IPCIO* ipcio, Message* buf, balance_t balance) {
  BalanceHistory history = {.s_id = ipc_id(ipcio), .s_history_len = 1};
  history.s_history[0] = (BalanceState){
      .s_balance = balance, .s_time = 0, .s_balance_pending_in = 0};

  child_sync_started(ipcio, buf, balance);

  while (1) {
    if (receive_any(ipcio, buf) == 0) {
      // fill in the missing balance history entries
      timestamp_t recv_t = get_physical_time();
      for (timestamp_t t = history.s_history_len; t < recv_t; ++t) {
        history.s_history[t] = (BalanceState){
            .s_balance = balance, .s_time = t, .s_balance_pending_in = 0};
        history.s_history_len++;
      }

      if (buf->s_header.s_type == STOP) {
        // insert the final balance entry
        history.s_history[recv_t] = (BalanceState){
            .s_balance = balance, .s_time = recv_t, .s_balance_pending_in = 0};
        history.s_history_len++;
        break;
      }
      if (buf->s_header.s_type == TRANSFER) {
        TransferOrder* transfer = (TransferOrder*)buf->s_payload;

        if (transfer->s_src == ipc_id(ipcio)) {
          balance -= transfer->s_amount;
          log_eventf(log_transfer_out_fmt, recv_t, transfer->s_src,
                     transfer->s_amount, transfer->s_dst);

          send(ipcio, transfer->s_dst, buf);
        } else if (transfer->s_dst == ipc_id(ipcio)) {
          balance += transfer->s_amount;
          log_eventf(log_transfer_in_fmt, recv_t, transfer->s_dst,
                     transfer->s_amount, transfer->s_src);

          buf->s_header.s_type = ACK;
          buf->s_header.s_payload_len = 0;
          send(ipcio, PARENT_ID, buf);
        }
      }
    }
  }

  child_sync_done(ipcio, buf, balance);

  buf->s_header.s_type = BALANCE_HISTORY;
  buf->s_header.s_payload_len = sizeof(BalanceHistory);
  *((BalanceHistory*)buf->s_payload) = history;
  send(ipcio, PARENT_ID, buf);
}

void child_sync_started(IPCIO* ipcio, Message* buf, balance_t balance) {
  ipc_ext_set_payload(buf, STARTED, log_started_fmt, 0, ipc_id(ipcio), getpid(),
                      getppid(), balance);
  if (send_multicast(ipcio, buf) != 0)
    log_panic(ipc_id(ipcio), "failed to broadcast the STARTED message\n");
  log_event(buf->s_payload);

  ipc_ext_await_all_started(ipcio, buf);
}

void child_sync_done(IPCIO* ipcio, Message* buf, balance_t balance) {
  ipc_ext_set_payload(buf, DONE, log_done_fmt, get_physical_time(),
                      ipc_id(ipcio), balance);
  if (send_multicast(ipcio, buf) != 0)
    log_panic(ipc_id(ipcio), "failed to broadcast the DONE message\n");
  log_event(buf->s_payload);

  ipc_ext_await_all_done(ipcio, buf);
}
