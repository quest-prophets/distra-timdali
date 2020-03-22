#pragma once

#include "ipcio.h"

void child_entry(IPCIO* ipcio);

void child_broadcast_started(IPCIO* ipcio, Message* buf);

void child_broadcast_done(IPCIO* ipcio, Message* buf);

void child_await_all_started(IPCIO* ipcio, Message* buf);

void child_await_all_done(IPCIO* ipcio, Message* buf);
