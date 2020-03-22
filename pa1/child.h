#pragma once

#include "ipcio.h"

void child_entry(IPCIO* ipcio, Message* buf);

void child_broadcast_started(IPCIO* ipcio, Message* buf);

void child_broadcast_done(IPCIO* ipcio, Message* buf);
