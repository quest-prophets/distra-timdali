#include "mutex.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ipcext.h"
#include "log.h"
#include "time.h"

/* Dining philosophers state */

typedef struct {
  bool has_fork[MAX_PROCESSES];
  bool fork_dirty[MAX_PROCESSES];
  bool has_request[MAX_PROCESSES];
} MutexState;

/* Mutual exclusion */

struct Mutex {
  local_id num_children;
  IPCIO* ipcio;
  Message* buf;
  MutexState st;
};

void request_fork(Mutex* mux, local_id from) {
  assert(mux->st.has_request[from]);
  mux->st.has_request[from] = false;

  advance_lamport_time();
  ipc_ext_set_status(mux->buf, CS_REQUEST, get_lamport_time());
  send(mux->ipcio, from, mux->buf);
}

void give_up_fork(Mutex* mux, local_id to) {
  assert(mux->st.has_fork[to]);
  mux->st.has_fork[to] = false;

  advance_lamport_time();
  ipc_ext_set_status(mux->buf, CS_REPLY, get_lamport_time());
  send(mux->ipcio, to, mux->buf);
}

int request_cs(const void* mutex) {
  Mutex* mux = (Mutex*)mutex;
  local_id self_id = ipc_id(mux->ipcio);

  while (1) {
    for (local_id c = PARENT_ID + 1; c <= mux->num_children; ++c) {
      if (c == self_id)
        continue;
      if (!mux->st.has_fork[c] && mux->st.has_request[c])
        request_fork(mux, c);
    }

    local_id from;

    if (ipc_receive_any(mux->ipcio, mux->buf, &from) == 0) {
      synchronize_time(mux->buf->s_header.s_local_time);

      if (mux->buf->s_header.s_type == CS_REPLY) {
        mux->st.has_fork[from] = true;
        mux->st.fork_dirty[from] = false;
      } else if (mux->buf->s_header.s_type == CS_REQUEST) {
        mux->st.has_request[from] = true;
        if (mux->st.has_fork[from] && mux->st.fork_dirty[from])
          give_up_fork(mux, from);
      } else {
        return NON_CS_MESSAGE_RECEIVED;
      }
    }

    bool can_enter_cs = true;
    for (local_id c = PARENT_ID + 1; c <= mux->num_children; ++c) {
      if (c == self_id)
        continue;
      if (!mux->st.has_fork[c] || (mux->st.fork_dirty[c] && mux->st.has_request[c])) {
        can_enter_cs = false;
        break;
      }
    }
    if (can_enter_cs)
      return 0;
  }
}

int release_cs(const void* mutex) {
  Mutex* mux = (Mutex*)mutex;

  for (local_id c = PARENT_ID + 1; c <= mux->num_children; ++c) {
    if (c == ipc_id(mux->ipcio))
      continue;

    mux->st.fork_dirty[c] = true;
    if (mux->st.has_request[c])
      give_up_fork(mux, c);
  }

  return 0;
}

int handle_cs_requests(Mutex* mux) {
  while (1) {
    local_id from;
    if (ipc_receive_any(mux->ipcio, mux->buf, &from) == 0) {
      synchronize_time(mux->buf->s_header.s_local_time);

      if (mux->buf->s_header.s_type == CS_REQUEST)
        give_up_fork(mux, from);
      else
        return NON_CS_MESSAGE_RECEIVED;
    }
  }
}

Mutex* mutex_init(IPCIO* ipcio, Message* buf, local_id num_children) {
  Mutex* mux = (Mutex*)calloc(1, sizeof(Mutex));
  mux->ipcio = ipcio;
  mux->buf = buf;
  mux->num_children = num_children;

  local_id self_id = ipc_id(ipcio);
  for (local_id c = PARENT_ID + 1; c <= num_children; ++c) {
    mux->st.has_fork[c] = mux->st.fork_dirty[c] = c < self_id;
    mux->st.has_request[c] = c > self_id;
  }

  return mux;
}
