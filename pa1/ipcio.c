#include "ipcio.h"

#include <errno.h>
#include <fcntl.h>
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

IPCIO* ipc_init(local_id num_children) {
  IPCIO* ipcio = (IPCIO*)calloc(1, sizeof(IPCIO));
  ipcio->id = PARENT_ID;
  ipcio->num_children = num_children;
  // total number of processes = num_children + 1
  for (local_id dst = 0; dst <= ipcio->num_children; ++dst) {
    for (local_id from = 0; from <= ipcio->num_children; ++from) {
      if (dst != from && pipe((int*)&ipcio->pipes[dst][from]) != 0) {
        free(ipcio);
        return NULL;
      }
    }
  }
  return ipcio;
}

void ipc_use_nonblocking_io(IPCIO* ipcio) {
  for (local_id dst = 0; dst <= ipcio->num_children; ++dst) {
    for (local_id from = 0; from <= ipcio->num_children; ++from) {
      PipeDescriptor pipe = ipcio->pipes[dst][from];
      if (pipe.read_fd > 0) {
        int flags = fcntl(pipe.read_fd, F_GETFL, 0);
        fcntl(pipe.read_fd, F_SETFL, flags | O_NONBLOCK);
      }
      if (pipe.write_fd > 0) {
        int flags = fcntl(pipe.write_fd, F_GETFL, 0);
        fcntl(pipe.write_fd, F_SETFL, flags | O_NONBLOCK);
      }
    }
  }
}

void _close_unused_pipes(IPCIO* ipcio) {
  // only leave pipes of type this->any for writing and any->this for reading
  for (local_id dst = 0; dst <= ipcio->num_children; ++dst) {
    for (local_id from = 0; from <= ipcio->num_children; ++from) {
      if (dst == from)
        continue;
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

void ipc_set_up_child(IPCIO* ipcio, local_id child_id) {
  ipcio->id = PARENT_ID + 1 + child_id;
  _close_unused_pipes(ipcio);
}

void ipc_set_up_parent(IPCIO* ipcio) {
  _close_unused_pipes(ipcio);
}

int ipc_read(const IPCIO* ipcio, local_id from, void* buf, size_t len) {
  // total number of processes = num_children + 1
  if (from > ipcio->num_children)
    return -1;
  int fd = ipcio->pipes[ipcio->id][from].read_fd;

  size_t total_read = 0;
  while (total_read < len) {
    errno = 0;
    ssize_t rd;
    if ((rd = read(fd, buf, len - total_read)) < 0) {
      if (errno == EAGAIN)
        if (total_read > 0)
          continue;  // block until we read the entire response
        else
          return IPC_ERR_NONBLOCK_EMPTY;
      else if (errno != 0)
        return -1;
    }
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
  if (*from > ipcio->num_children)
    return RECEIVE_ALL_END;

  return receive_blocking(ipcio, *from, buf);
}

void ipc_iterate_pipes(const IPCIO* ipcio, PipeIterator callback) {
  for (local_id dst = 0; dst <= ipcio->num_children; ++dst)
    for (local_id from = 0; from <= ipcio->num_children; ++from)
      if (ipcio->pipes[dst][from].read_fd != 0)
        callback(dst, from, ipcio->pipes[dst][from].read_fd,
                 ipcio->pipes[dst][from].write_fd);
}

int receive_blocking(IPCIO* ipcio, local_id from, Message* buf) {
  int err;
  while ((err = receive(ipcio, from, buf)) == IPC_ERR_NONBLOCK_EMPTY)
    ;
  return err;
}
