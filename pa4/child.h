#pragma once

#include "ipcext.h"

void child_entry(IPCIO* ipcio, Message* buf);

void child_sync_started(IPCIO* ipcio, Message* buf);

void child_sync_done(IPCIO* ipcio, Message* buf);
