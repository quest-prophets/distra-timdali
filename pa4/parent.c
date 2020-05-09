#include "parent.h"

#include <assert.h>
#include <stdlib.h>

#include "log.h"
#include "time.h"

void parent_entry(IPCIO* ipcio, Message* buf) {
  ipc_ext_await_all_started(ipcio, buf);
  ipc_ext_await_all_done(ipcio, buf);
}
