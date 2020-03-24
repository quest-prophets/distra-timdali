#include "child.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ipcext.h"
#include "log.h"
#include "pa1.h"

void child_entry(IPCIO* ipcio, Message* buf) {
  child_broadcast_started(ipcio, buf);
  ipc_ext_await_all_started(ipcio, buf);

  child_broadcast_done(ipcio, buf);
  ipc_ext_await_all_done(ipcio, buf);
}

void child_broadcast_started(IPCIO* ipcio, Message* buf) {
  ipc_ext_set_payload(buf, STARTED, log_started_fmt, ipc_id(ipcio), getpid(),
                      getppid());
  if (send_multicast(ipcio, buf) != 0)
    log_panic(ipc_id(ipcio), "failed to broadcast the STARTED message\n");
  log_event(buf->s_payload);
}

void child_broadcast_done(IPCIO* ipcio, Message* buf) {
  ipc_ext_set_payload(buf, DONE, log_done_fmt, ipc_id(ipcio));
  if (send_multicast(ipcio, buf) != 0)
    log_panic(ipc_id(ipcio), "failed to broadcast the DONE message\n");
  log_event(buf->s_payload);
}
