#pragma once

#include "ipcext.h"
#include "banking.h"

void child_entry(IPCIO* ipcio, Message* buf, balance_t balance);

void child_sync_started(IPCIO* ipcio, Message* buf, balance_t balance);

void child_sync_done(IPCIO* ipcio, Message* buf, balance_t balance);
