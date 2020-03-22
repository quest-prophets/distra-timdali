#include "ipcio.h"

#include <unistd.h>

int ipc_init(IPCIO* ipcio) {
  // total number of processes = num_children + 1
  for (uint8_t dst = 0; dst <= ipcio->num_children; ++dst)
    for (uint8_t from = 0; from <= ipcio->num_children; ++from)
      if (dst != from && pipe((int*)&ipcio->pipes[dst][from]) != 0)
        return -1;
  return 0;
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

void ipc_iterate_pipes(const IPCIO* ipcio, PipeIterator callback) {
  for (uint8_t dst = 0; dst <= ipcio->num_children; ++dst)
    for (uint8_t from = 0; from <= ipcio->num_children; ++from)
      if (ipcio->pipes[dst][from].read_fd != 0)
        callback(dst, from, ipcio->pipes[dst][from].read_fd,
                 ipcio->pipes[dst][from].write_fd);
}
