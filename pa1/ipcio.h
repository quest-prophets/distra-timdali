#pragma once

#include "ipc.h"

#include <stdio.h>

#define MAX_PROCESSES (MAX_PROCESS_ID + 1)
#define MAX_CHILDREN (MAX_PROCESSES - 1)

typedef struct {
  int read_fd, write_fd;
} __attribute__((packed)) PipeDescriptor;

typedef struct {
  local_id id;
  local_id num_children;
  PipeDescriptor pipes[MAX_PROCESSES][MAX_PROCESSES];
} IPCIO;

int ipc_init(IPCIO* ipcio);

int ipc_read(const IPCIO* ipcio, local_id from, void* buf, size_t len);

int ipc_write(const IPCIO* ipcio, local_id dst, const void* buf, size_t len);

typedef void (*PipeIterator)(local_id dst, local_id from, int rd_fd, int wr_fd);

void ipc_iterate_pipes(const IPCIO* ipcio, PipeIterator callback);
