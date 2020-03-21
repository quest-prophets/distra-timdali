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
}
