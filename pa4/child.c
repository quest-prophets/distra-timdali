#include "child.h"

#include <stdio.h>
#include <unistd.h>

#include "log.h"
#include "pa2345.h"
#include "time.h"

void child_entry(IPCIO* ipcio, Message* buf) {
  child_sync_started(ipcio, buf);

  while (1) {
    if (receive_any(ipcio, buf) == 0) {
      synchronize_time(buf->s_header.s_local_time);
      timestamp_t recv_t = get_lamport_time();

      // OwO wip
    }
  }

  child_sync_done(ipcio, buf);
}

void child_sync_started(IPCIO* ipcio, Message* buf) {
  advance_lamport_time();
  timestamp_t time = get_lamport_time();
  ipc_ext_set_payload(buf, STARTED, time, log_started_fmt, time, ipc_id(ipcio), getpid(), getppid(), 0);
  if (send_multicast(ipcio, buf) != 0)
    log_panic(ipc_id(ipcio), "failed to broadcast the STARTED message\n");
  log_event(buf->s_payload);

  ipc_ext_await_all_started(ipcio, buf);
}

void child_sync_done(IPCIO* ipcio, Message* buf) {
  advance_lamport_time();
  timestamp_t time = get_lamport_time();
  ipc_ext_set_payload(buf, DONE, time, log_done_fmt, time, ipc_id(ipcio), 0);
  if (send_multicast(ipcio, buf) != 0)
    log_panic(ipc_id(ipcio), "failed to broadcast the DONE message\n");
  log_event(buf->s_payload);

  ipc_ext_await_all_done(ipcio, buf);
}
