#include "ipcio.h"

#include <stdlib.h>
#include <unistd.h>

typedef struct {
  int read_fd, write_fd;
} __attribute__((packed)) PipeDescriptor;

struct IPCIO {
  local_id id;
  local_id num_children;
  PipeDescriptor pipes[MAX_PROCESSES][MAX_PROCESSES];
};

/* ipc.h */

int send(void* self, local_id dst, const Message* msg) {
  IPCIO* ipcio = (IPCIO*)self;

  size_t len = sizeof(MessageHeader) + msg->s_header.s_payload_len;
  return ipc_write(ipcio, dst, msg, len);
}

int receive(void* self, local_id from, Message* msg) {
  IPCIO* ipcio = (IPCIO*)self;

  int err = 0;
  if ((err = ipc_read(ipcio, from, &msg->s_header, sizeof(MessageHeader))) != 0)
    return err;
  err = ipc_read(ipcio, from, msg->s_payload, msg->s_header.s_payload_len);

  return err;
}

int send_multicast(void* self, const Message* msg) {
  IPCIO* ipcio = (IPCIO*)self;

  int err = 0;
  for (local_id dst = 0; dst <= ipcio->num_children; ++dst)
    if (dst != ipcio->id && (err = send(self, dst, msg)) != 0)
      break;

  return err;
}

int receive_any(void* self, Message* msg) {
  IPCIO* ipcio = (IPCIO*)self;

  for (local_id from = 0; from <= ipcio->num_children; ++from)
    if (from != ipcio->id && receive(self, from, msg) == 0)
      return 0;

  return -1;
}

/* ipcio.h */

local_id ipc_id(const IPCIO* ipcio) {
  return ipcio->id;
}

IPCIO* ipc_init_parent(local_id num_children) {
  IPCIO* ipcio = (IPCIO*)calloc(1, sizeof(IPCIO));
  ipcio->id = PARENT_ID;
  ipcio->num_children = num_children;
  // total number of processes = num_children + 1
  for (uint8_t dst = 0; dst <= ipcio->num_children; ++dst) {
    for (uint8_t from = 0; from <= ipcio->num_children; ++from) {
      if (dst != from && pipe((int*)&ipcio->pipes[dst][from]) != 0) {
        free(ipcio);
        return NULL;
      }
    }
  }
  return ipcio;
}

void ipc_init_child(IPCIO* ipcio, local_id child_id) {
  ipcio->id = PARENT_ID + 1 + child_id;
  // close unused pipes
  for (uint8_t dst = PARENT_ID + 1; dst <= ipcio->num_children; ++dst) {
    for (uint8_t from = PARENT_ID + 1; from <= ipcio->num_children; ++from) {
      if (dst != ipcio->id) {
        close(ipcio->pipes[dst][from].read_fd);
        ipcio->pipes[dst][from].read_fd = -1;
      }
      if (from != ipcio->id) {
        close(ipcio->pipes[dst][from].write_fd);
        ipcio->pipes[dst][from].write_fd = -1;
      }
    }
  }
}

int ipc_read(const IPCIO* ipcio, local_id from, void* buf, size_t len) {
  // total number of processes = num_children + 1
  if (from > ipcio->num_children)
    return -1;
  int fd = ipcio->pipes[ipcio->id][from].read_fd;

  size_t total_read = 0;
  while (total_read < len) {
    ssize_t rd;
    if ((rd = read(fd, buf, len - total_read)) < 0)
      return -1;
    total_read += rd;
  }
  return 0;
}

int ipc_write(const IPCIO* ipcio, local_id dst, const void* buf, size_t len) {
  // total number of processes = num_children + 1
  if (dst > ipcio->num_children)
    return -1;
  return write(ipcio->pipes[dst][ipcio->id].write_fd, buf, len) != len;
}

void ipc_receive_all_children_start(local_id* from) {
  *from = PARENT_ID;
}

int ipc_receive_all_next(IPCIO* ipcio, local_id* from, Message* buf) {
  *from += 1;
  if (*from == ipcio->id)
    *from += 1;
  if (*from >= ipcio->num_children)
    return RECEIVE_ALL_END;

  return receive(ipcio, *from, buf);
}

void ipc_iterate_pipes(const IPCIO* ipcio, PipeIterator callback) {
  for (uint8_t dst = 0; dst <= ipcio->num_children; ++dst)
    for (uint8_t from = 0; from <= ipcio->num_children; ++from)
      if (ipcio->pipes[dst][from].read_fd != 0)
        callback(dst, from, ipcio->pipes[dst][from].read_fd,
                 ipcio->pipes[dst][from].write_fd);
}
