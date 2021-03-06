#pragma once

#include "ipcio.h"

void ipc_ext_set_payload(Message* buf, MessageType type, const char* fmt, ...);

void ipc_ext_receive_all_type(IPCIO* ipcio, Message* buf, MessageType type);

void ipc_ext_await_all_started(IPCIO* ipcio, Message* buf);

void ipc_ext_await_all_done(IPCIO* ipcio, Message* buf);
