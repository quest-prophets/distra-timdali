#include "mutex.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ipcext.h"
#include "log.h"
#include "time.h"

/* Request queue */

/* The queue is based  on the following assumptions:
 * - each process can send at most one request for the CS before releasing it
 * - lamport clock starts at 1, so 0 can be reserved to indicate "no requests"
 * - there's always at least one (own) request in the queue
 */

#define EMPTY_REQUEST_TIME 0

typedef struct {
  timestamp_t ts[MAX_CHILDREN];
} RequestQueue;

local_id top_enqueued_request(const RequestQueue* q);

timestamp_t queue_lookup(const RequestQueue* q, local_id id);

bool queue_contains(const RequestQueue* q, local_id from);

void enqueue_request(RequestQueue* q, local_id from, timestamp_t t);

void release_request(RequestQueue* q, local_id from);

/* Mutual exclusion */

struct Mutex {
  IPCIO* ipcio;
  Message* buf;
  RequestQueue queue;
  local_id num_children;
  int last_request_reply_count;
};

timestamp_t enqueue_own_request(Mutex* mux) {
  if (queue_contains(&mux->queue, ipc_id(mux->ipcio)))
    return queue_lookup(&mux->queue, ipc_id(mux->ipcio));

  advance_lamport_time();
  timestamp_t t = get_lamport_time();
  enqueue_request(&mux->queue, ipc_id(mux->ipcio), t);

  ipc_ext_set_status(mux->buf, CS_REQUEST, t);
  if (ipc_multicast_to_children(mux->ipcio, mux->buf) != 0)
    log_panic(ipc_id(mux->ipcio), "failed to broadcast the CS_REQUEST message\n");

  return t;
}

int request_cs(const void* mutex) {
  Mutex* mux = (Mutex*)mutex;
  timestamp_t own_request_time = enqueue_own_request(mux);

  while (1) {
    local_id from;
    if (ipc_receive_any(mux->ipcio, mux->buf, &from) == 0) {
      synchronize_time(mux->buf->s_header.s_local_time);

      if (mux->buf->s_header.s_type == CS_REPLY) {
        mux->last_request_reply_count++;
      } else if (mux->buf->s_header.s_type == CS_REQUEST) {
        timestamp_t request_time = mux->buf->s_header.s_local_time;

        if (own_request_time < request_time ||
            (own_request_time == request_time && ipc_id(mux->ipcio) < from)) {
          enqueue_request(&mux->queue, from, request_time);
        } else {
          advance_lamport_time();
          ipc_ext_set_status(mux->buf, CS_REPLY, get_lamport_time());
          send(mux->ipcio, from, mux->buf);
        }
      } else {
        return NON_CS_MESSAGE_RECEIVED;
      }

      if (mux->last_request_reply_count == mux->num_children - 1)
        break;
    }
  }

  mux->last_request_reply_count = 0;
  return 0;
}

int release_cs(const void* mutex) {
  Mutex* mux = (Mutex*)mutex;

  release_request(&mux->queue, ipc_id(mux->ipcio));

  for (int from = PARENT_ID + 1; from <= mux->num_children; ++from) {
    if (queue_contains(&mux->queue, from)) {
      release_request(&mux->queue, from);

      advance_lamport_time();
      ipc_ext_set_status(mux->buf, CS_REPLY, get_lamport_time());
      send(mux->ipcio, from, mux->buf);
    }
  }

  return 0;
}

int handle_cs_requests(Mutex* mux) {
  while (1) {
    local_id from;
    if (ipc_receive_any(mux->ipcio, mux->buf, &from) == 0) {
      synchronize_time(mux->buf->s_header.s_local_time);
      if (mux->buf->s_header.s_type == CS_REQUEST) {
        advance_lamport_time();
        ipc_ext_set_status(mux->buf, CS_REPLY, get_lamport_time());
        send(mux->ipcio, from, mux->buf);
      } else {
        return NON_CS_MESSAGE_RECEIVED;
      }
    }
  }
}

Mutex* mutex_init(IPCIO* ipcio, Message* buf, local_id num_children) {
  Mutex* mux = (Mutex*)calloc(1, sizeof(Mutex));
  mux->ipcio = ipcio;
  mux->buf = buf;
  mux->num_children = num_children;
  return mux;
}

/* Request queue */

local_id top_enqueued_request(const RequestQueue* q) {
  local_id top_enqueued_child = 0;
  timestamp_t top_enqueued_ts = q->ts[0];
  for (int i = 0; i < MAX_CHILDREN; ++i) {
    if (q->ts[i] == EMPTY_REQUEST_TIME)
      continue;
    if (top_enqueued_ts == EMPTY_REQUEST_TIME || q->ts[i] < top_enqueued_ts) {
      top_enqueued_child = i;
      top_enqueued_ts = q->ts[i];
    }
  }
  return top_enqueued_child + 1;
}

timestamp_t queue_lookup(const RequestQueue* q, local_id id) {
  return q->ts[id - 1];
}

bool queue_contains(const RequestQueue* q, local_id from) {
  return queue_lookup(q, from) != EMPTY_REQUEST_TIME;
}

void enqueue_request(RequestQueue* q, local_id from, timestamp_t t) {
  assert(!queue_contains(q, from));
  q->ts[from - 1] = t;
}

void release_request(RequestQueue* q, local_id from) {
  assert(queue_contains(q, from));
  q->ts[from - 1] = EMPTY_REQUEST_TIME;
}
