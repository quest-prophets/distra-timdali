#include "child.h"

#include <stdio.h>
#include <unistd.h>

#include "pa1.h"

void child_start(IPCIO ipcio) {
  printf(log_started_fmt, ipcio.id, getpid(), getppid());
}
