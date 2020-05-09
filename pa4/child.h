#pragma once

#include "ipcext.h"
#include <stdbool.h>

void child_entry(IPCIO* ipcio, Message* buf, local_id num_children, bool mutexl);

void child_sync_started(IPCIO* ipcio, Message* buf);

void child_send_done(IPCIO* ipcio, Message* buf);
