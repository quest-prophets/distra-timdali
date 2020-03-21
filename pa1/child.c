#include "child.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ipc.h"
#include "pa1.h"

void child_start(IPCIO ipcio) {
  Message msg = {.s_header = {.s_magic = MESSAGE_MAGIC,
                              .s_payload_len = 0,
                              .s_type = STARTED,
                              .s_local_time = 0}};
  if (send_multicast(&ipcio, &msg) != 0) {
    fprintf(stderr, "Process %d failed to broadcast the STARTED message\n",
            ipcio.id);
    exit(1);
  }
  printf(log_started_fmt, ipcio.id, getpid(), getppid());

  for (local_id from = PARENT_ID + 1; from <= ipcio.num_children; ++from) {
    if (from == ipcio.id)
      continue;
    if (receive(&ipcio, from, &msg) != 0) {
      fprintf(stderr,
              "Process %d failed to receive a message from process %d\n",
              ipcio.id, from);
      exit(1);
    }
    if (msg.s_header.s_type != STARTED) {
      fprintf(stderr,
              "Process %d received an unexpected message from process %d "
              "(expected STARTED, got %d)\n",
              ipcio.id, from, msg.s_header.s_type);
      exit(1);
    }
  }
  printf(log_received_all_started_fmt, ipcio.id);
}
