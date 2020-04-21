#include "ipcext.h"

#include <stdio.h>

#include "banking.h"
#include "log.h"
#include "pa2345.h"
#include "time.h"

static char* MESSAGE_TYPE_STR[] = {"STARTED",    "DONE",     "ACK",
                                   "STOP",       "TRANSFER", "BALANCE_HISTORY",
                                   "CS_REQUEST", "CS_REPLY", "CS_RELEASE"};
static size_t MESSAGE_TYPE_CNT =
    sizeof(MESSAGE_TYPE_STR) / sizeof(MESSAGE_TYPE_STR[0]);

void ipc_ext_set_status(Message* buf, MessageType type, timestamp_t time) {
  buf->s_header.s_magic = MESSAGE_MAGIC;
  buf->s_header.s_type = type;
  buf->s_header.s_local_time = time;
}

void ipc_ext_set_payload(Message* buf, MessageType type, timestamp_t time, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  ipc_ext_set_status(buf, type, time);
  buf->s_header.s_payload_len =
      vsnprintf(buf->s_payload, MAX_MESSAGE_LEN, fmt, args);
  va_end(args);
}

void ipc_ext_receive_all_type(IPCIO* ipcio, Message* buf, MessageType type) {
  ipc_ext_receive_all(ipcio, buf, type, NULL, NULL);
}

void ipc_ext_receive_all(IPCIO* ipcio,
                         Message* buf,
                         MessageType type,
                         void* callback_data,
                         void (*callback)(void*, local_id, Message*)) {
  local_id from;
  int err;

  ipc_receive_all_children_start(&from);
  while ((err = ipc_receive_all_next(ipcio, &from, buf)) != RECEIVE_ALL_END) {
    if (err != 0)
      log_panic(ipc_id(ipcio), "failed to read from process %d\n", from);

    MessageType buf_type = buf->s_header.s_type;
    if (buf_type != type)
      log_panic(ipc_id(ipcio),
                "received an unexpected message from process %d "
                "(expected %d = %s, got %d = %s)\n",
                from, type, MESSAGE_TYPE_STR[type], buf_type,
                buf_type < MESSAGE_TYPE_CNT ? MESSAGE_TYPE_STR[buf_type]
                                            : "<unknown>");

    synchronize_time(buf->s_header.s_local_time);

    if (callback != NULL)
      callback(callback_data, from, buf);
  }
}

void ipc_ext_await_all_started(IPCIO* ipcio, Message* buf) {
  ipc_ext_receive_all_type(ipcio, buf, STARTED);
  log_eventf(log_received_all_started_fmt, buf->s_header.s_local_time, ipc_id(ipcio));
}

void ipc_ext_await_all_done(IPCIO* ipcio, Message* buf) {
  ipc_ext_receive_all_type(ipcio, buf, DONE);
  log_eventf(log_received_all_done_fmt, buf->s_header.s_local_time, ipc_id(ipcio));
}
