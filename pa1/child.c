#include "child.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "errors.h"
#include "ipc.h"
#include "pa1.h"

void child_start(IPCIO ipcio) {
  Message msg = {.s_header = {.s_magic = MESSAGE_MAGIC,
                              .s_payload_len = 0,
                              .s_type = STARTED,
                              .s_local_time = 0}};
  if (send_multicast(&ipcio, &msg) != 0)
    panic(ipcio.id, "failed to broadcast the STARTED message\n");
  printf(log_started_fmt, ipcio.id, getpid(), getppid());

  for (local_id from = PARENT_ID + 1; from <= ipcio.num_children; ++from) {
    if (from == ipcio.id)
      continue;
    if (receive(&ipcio, from, &msg) != 0)
      panic(ipcio.id, "failed to receive a message from process %d\n", from);
    if (msg.s_header.s_type != STARTED)
      panic(ipcio.id,
            "received an unexpected message from process %d "
            "(expected STARTED, got %d)\n",
            from, msg.s_header.s_type);
  }
  printf(log_received_all_started_fmt, ipcio.id);

  msg.s_header.s_type = DONE;
  if (send_multicast(&ipcio, &msg) != 0)
    panic(ipcio.id, "failed to broadcast the DONE message\n");
  printf(log_done_fmt, ipcio.id);

  for (local_id from = PARENT_ID + 1; from <= ipcio.num_children; ++from) {
    if (from == ipcio.id)
      continue;
    if (receive(&ipcio, from, &msg) != 0)
      panic(ipcio.id, "failed to receive a message from process %d\n", from);
    if (msg.s_header.s_type != DONE)
      panic(ipcio.id,
            "received an unexpected message from process %d "
            "(expected DONE, got %d)\n",
            from, msg.s_header.s_type);
  }
  printf(log_received_all_done_fmt, ipcio.id);
}
