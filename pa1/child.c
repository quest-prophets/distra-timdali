#include "child.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ipc.h"
#include "log.h"
#include "pa1.h"

void child_entry(IPCIO* ipcio) {
  log_init_events_log();

  Message* msg = calloc(1, sizeof(Message));
  msg->s_header.s_magic = MESSAGE_MAGIC;

  child_broadcast_started(ipcio, msg);
  child_await_all_started(ipcio, msg);

  child_broadcast_done(ipcio, msg);
  child_await_all_done(ipcio, msg);
}

void msg_set_payload(Message* msg, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  msg->s_header.s_payload_len =
      vsnprintf(msg->s_payload, MAX_MESSAGE_LEN, fmt, args);
}

void child_broadcast_started(IPCIO* ipcio, Message* buf) {
  buf->s_header.s_type = STARTED;
  msg_set_payload(buf, log_started_fmt, ipc_id(ipcio), getpid(), getppid());
  if (send_multicast(ipcio, buf) != 0)
    log_panic(ipc_id(ipcio), "failed to broadcast the STARTED message\n");
  log_event(buf->s_payload);
}

void child_broadcast_done(IPCIO* ipcio, Message* buf) {
  buf->s_header.s_type = DONE;
  msg_set_payload(buf, log_done_fmt, ipc_id(ipcio));
  if (send_multicast(ipcio, buf) != 0)
    log_panic(ipc_id(ipcio), "failed to broadcast the DONE message\n");
  log_event(buf->s_payload);
}

void child_await_all_started(IPCIO* ipcio, Message* buf) {
  local_id from;
  int status;

  ipc_receive_all_children_start(&from);
  while ((status = ipc_receive_all_next(ipcio, &from, buf)) !=
         RECEIVE_ALL_END) {
    if (status != 0)
      log_panic(ipc_id(ipcio), "failed to receive a message from process %d\n",
                from);
    if (buf->s_header.s_type != STARTED)
      log_panic(ipc_id(ipcio),
                "received an unexpected message from process %d "
                "(expected STARTED, got %d)\n",
                from, buf->s_header.s_type);
  }
  log_eventf(log_received_all_started_fmt, ipc_id(ipcio));
}

void child_await_all_done(IPCIO* ipcio, Message* buf) {
  local_id from;
  int status;

  ipc_receive_all_children_start(&from);
  while ((status = ipc_receive_all_next(ipcio, &from, buf)) !=
         RECEIVE_ALL_END) {
    if (status != 0)
      log_panic(ipc_id(ipcio), "failed to receive a message from process %d\n",
                from);
    if (buf->s_header.s_type != DONE)
      log_panic(ipc_id(ipcio),
                "received an unexpected message from process %d "
                "(expected DONE, got %d)\n",
                from, buf->s_header.s_type);
  }
  log_eventf(log_received_all_done_fmt, ipc_id(ipcio));
}
