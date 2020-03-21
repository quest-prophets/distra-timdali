#include "ipc.h"

#include "ipcio.h"

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
    if ((err = send(self, dst, msg)) != 0)
      break;

  return err;
}

int receive_any(void* self, Message* msg) {
  IPCIO* ipcio = (IPCIO*)self;

  for (local_id from = 0; from <= ipcio->num_children; ++from)
    if (receive(self, from, msg) == 0)
      return 0;

  return -1;
}
