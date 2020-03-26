#include "parent.h"

#include <assert.h>
#include <stdlib.h>

#include "banking.h"
#include "log.h"

void transfer(void* parent_data, local_id src, local_id dst, balance_t amount) {
  IPCIO* ipcio = (IPCIO*)parent_data;
  Message* buf = malloc(sizeof(Message));

  buf->s_header = (MessageHeader){.s_magic = MESSAGE_MAGIC,
                                  .s_type = TRANSFER,
                                  .s_payload_len = sizeof(TransferOrder),
                                  .s_local_time = get_physical_time()};
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
