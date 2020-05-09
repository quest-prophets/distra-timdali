#include "child.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "log.h"
#include "mutex.h"
#include "pa2345.h"
#include "time.h"

void child_entry(IPCIO* ipcio, Message* buf, local_id num_children) {
  Mutex* mutex = mutex_init(ipcio, buf, num_children);
  child_sync_started(ipcio, buf);

  int awaiting_children = num_children - 1;

  int iterations_count = 5 * ipc_id(ipcio);
  for (int i = 1; i <= iterations_count; ++i) {
    while (request_cs(mutex) == NON_CS_MESSAGE_RECEIVED) {
      ipc_ext_assert_message_type(ipcio, buf, DONE);
      awaiting_children--;
    }

    char s[256];
    snprintf(s, 256, log_loop_operation_fmt, ipc_id(ipcio), i, iterations_count);
    print(s);

    release_cs(mutex);
  }

  Message* done_buf = malloc(sizeof(Message));
  child_send_done(ipcio, done_buf);

  while (awaiting_children > 0) {
    handle_cs_requests(mutex);
    ipc_ext_assert_message_type(ipcio, buf, DONE);
    awaiting_children--;
  }

  // Log "process x has DONE ..."
  log_event(done_buf->s_payload);

  // Wait for other processes to receive DONE from us
  sleep(1);
}

void child_sync_started(IPCIO* ipcio, Message* buf) {
  advance_lamport_time();
  timestamp_t time = get_lamport_time();
  ipc_ext_set_payload(buf, STARTED, time, log_started_fmt, time, ipc_id(ipcio), getpid(), getppid(),
                      0);
  if (send_multicast(ipcio, buf) != 0)
    log_panic(ipc_id(ipcio), "failed to broadcast the STARTED message\n");
  log_event(buf->s_payload);

  ipc_ext_await_all_started(ipcio, buf);
}

void child_send_done(IPCIO* ipcio, Message* buf) {
  advance_lamport_time();
  timestamp_t time = get_lamport_time();
  ipc_ext_set_payload(buf, DONE, time, log_done_fmt, time, ipc_id(ipcio), 0);
  if (send_multicast(ipcio, buf) != 0)
    log_panic(ipc_id(ipcio), "failed to broadcast the DONE message\n");
}
