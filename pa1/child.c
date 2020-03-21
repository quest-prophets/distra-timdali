#include "child.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ipc.h"
#include "log.h"
#include "pa1.h"

void set_msg_payload(Message* msg, MessageType type, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  msg->s_header.s_type = type;
  msg->s_header.s_payload_len =
      vsnprintf(msg->s_payload, MAX_MESSAGE_LEN, fmt, args);
}

void child_start(IPCIO ipcio) {
  FILE* log_file = log_open_file();
  Message* msg = calloc(1, sizeof(Message));
  msg->s_header.s_magic = MESSAGE_MAGIC;

  set_msg_payload(msg, STARTED, log_started_fmt, ipcio.id, getpid(), getppid());
  if (send_multicast(&ipcio, msg) != 0)
    panic(ipcio.id, "failed to broadcast the STARTED message\n");
  log_write(log_file, msg->s_payload);

  for (local_id from = PARENT_ID + 1; from <= ipcio.num_children; ++from) {
    if (from == ipcio.id)
      continue;
    if (receive(&ipcio, from, msg) != 0)
      panic(ipcio.id, "failed to receive a message from process %d\n", from);
    if (msg->s_header.s_type != STARTED)
      panic(ipcio.id,
            "received an unexpected message from process %d "
            "(expected STARTED, got %d)\n",
            from, msg->s_header.s_type);
  }
  log_print(log_file, log_received_all_started_fmt, ipcio.id);

  set_msg_payload(msg, DONE, log_done_fmt, ipcio.id);
  if (send_multicast(&ipcio, msg) != 0)
    panic(ipcio.id, "failed to broadcast the DONE message\n");
  log_write(log_file, msg->s_payload);

  for (local_id from = PARENT_ID + 1; from <= ipcio.num_children; ++from) {
    if (from == ipcio.id)
      continue;
    if (receive(&ipcio, from, msg) != 0)
      panic(ipcio.id, "failed to receive a message from process %d\n", from);
    if (msg->s_header.s_type != DONE)
      panic(ipcio.id,
            "received an unexpected message from process %d "
            "(expected DONE, got %d)\n",
            from, msg->s_header.s_type);
  }
  log_print(log_file, log_received_all_done_fmt, ipcio.id);
}
