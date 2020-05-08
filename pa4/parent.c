#include "parent.h"

#include <assert.h>
#include <stdlib.h>

#include "log.h"
#include "time.h"

void parent_entry(IPCIO* ipcio, Message* buf) {
  ipc_ext_await_all_started(ipcio, buf);

  advance_lamport_time();
  ipc_ext_set_status(buf, STOP, get_lamport_time());
  if (send_multicast(ipcio, buf) != 0)
    log_panic(ipc_id(ipcio), "failed to broadcast the STOP message\n");

  ipc_ext_await_all_done(ipcio, buf);
}
