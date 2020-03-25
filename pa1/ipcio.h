#pragma once

#include "ipc.h"

#define MAX_PROCESSES (MAX_PROCESS_ID + 1)
#define MAX_CHILDREN (MAX_PROCESSES - 1)

typedef struct IPCIO IPCIO;

local_id ipc_id(const IPCIO* ipcio);

IPCIO* ipc_init(local_id num_children);

void ipc_use_nonblocking_io(IPCIO* ipcio);

void ipc_set_up_child(IPCIO* ipcio, local_id child_id);

void ipc_set_up_parent(IPCIO* ipcio);

int ipc_read(const IPCIO* ipcio, local_id from, void* buf, size_t len);

int ipc_write(const IPCIO* ipcio, local_id dst, const void* buf, size_t len);

#define RECEIVE_ALL_END 1

void ipc_receive_all_children_start(local_id* from);

int ipc_receive_all_next(IPCIO* ipcio, local_id* from, Message* buf);

// Logging

typedef void (*PipeIterator)(local_id dst, local_id from, int rd_fd, int wr_fd);

void ipc_iterate_pipes(const IPCIO* ipcio, PipeIterator callback);

// Non-blocking IO

#define IPC_ERR_NONBLOCK_EMPTY (-128)

// Equivalent to receive() when using blocking IO (PA1)
int receive_blocking(IPCIO* ipcio, local_id from, Message* buf);
