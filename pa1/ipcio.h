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

int ipc_read(const IPCIO* ipcio, local_id from, void* buf, size_t len);

int ipc_write(const IPCIO* ipcio, local_id dst, const void* buf, size_t len);
