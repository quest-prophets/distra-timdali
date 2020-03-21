#pragma once

#include "ipc.h"

#define MAX_CHILDREN MAX_PROCESS_ID

typedef struct {
  int read_fd, write_fd;
} __attribute__((packed)) PipeDescriptor;

typedef struct {
  local_id id;
  local_id num_children;
  PipeDescriptor pipes[MAX_CHILDREN];
} IPCIO;
