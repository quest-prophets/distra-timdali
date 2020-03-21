#include "ipcio.h"

#include <unistd.h>

int ipc_read(const IPCIO* ipcio, local_id from, void* buf, size_t len) {
  // total number of processes = num_children + 1
  if (from > ipcio->num_children)
    return -1;
  int fd = ipcio->pipes[from].read_fd;

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
  return write(ipcio->pipes[dst].write_fd, buf, len) != len;
}
