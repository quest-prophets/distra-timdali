#pragma once

#include "ipcio.h"

void ipc_ext_set_status(Message* buf, MessageType type, timestamp_t time);

void ipc_ext_set_payload(Message* buf, MessageType type, timestamp_t time, const char* fmt, ...);

void ipc_ext_receive_all_type(IPCIO* ipcio, Message* buf, MessageType type);

void ipc_ext_receive_all(IPCIO* ipcio,
                         Message* buf,
                         MessageType type,
                         void* callback_data,
                         void (*callback)(void*, local_id, Message*));

void ipc_ext_await_all_started(IPCIO* ipcio, Message* buf);

void ipc_ext_await_all_done(IPCIO* ipcio, Message* buf);

void ipc_ext_assert_message_type(IPCIO* ipcio, Message* buf, MessageType type);
