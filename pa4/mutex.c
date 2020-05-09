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

int request_cs(const void* mutex) {
  Mutex* mux = (Mutex*)mutex;
  advance_lamport_time();
  timestamp_t time = get_lamport_time();

  if (!queue_contains(&mux->queue, ipc_id(mux->ipcio))) {
    enqueue_request(&mux->queue, ipc_id(mux->ipcio), time);
    ipc_ext_set_status(mux->buf, CS_REQUEST, time);
    if (ipc_multicast_to_children(mux->ipcio, mux->buf) != 0)
      log_panic(ipc_id(mux->ipcio), "failed to broadcast the CS_REQUEST message\n");
  }

  while (1) {
    local_id from;
    if (ipc_receive_any(mux->ipcio, mux->buf, &from) == 0) {
      synchronize_time(mux->buf->s_header.s_local_time);

      if (mux->buf->s_header.s_type == CS_REPLY) {
        mux->last_request_reply_count++;
      } else if (mux->buf->s_header.s_type == CS_REQUEST) {
        timestamp_t request_time = mux->buf->s_header.s_local_time;
        enqueue_request(&mux->queue, from, request_time);

        advance_lamport_time();
        ipc_ext_set_status(mux->buf, CS_REPLY, get_lamport_time());
        send(mux->ipcio, from, mux->buf);
      } else if (mux->buf->s_header.s_type == CS_RELEASE) {
        release_request(&mux->queue, from);
      } else {
        return NON_CS_MESSAGE_RECEIVED;
      }

      if (mux->last_request_reply_count == mux->num_children - 1 &&
          top_enqueued_request(&mux->queue) == ipc_id(mux->ipcio))
        break;
    }
  }

  mux->last_request_reply_count = 0;
  return 0;
}

int release_cs(const void* mutex) {
  Mutex* mux = (Mutex*)mutex;

  release_request(&mux->queue, ipc_id(mux->ipcio));

  advance_lamport_time();
  ipc_ext_set_status(mux->buf, CS_RELEASE, get_lamport_time());
  if (ipc_multicast_to_children(mux->ipcio, mux->buf) != 0)
    log_panic(ipc_id(mux->ipcio), "failed to broadcast the CS_RELEASE message\n");

  return 0;
}

int handle_cs_requests(Mutex* mux) {
  while (1) {
    local_id from;
    if (ipc_receive_any(mux->ipcio, mux->buf, &from) == 0) {
      synchronize_time(mux->buf->s_header.s_local_time);
      if (mux->buf->s_header.s_type == CS_REQUEST) {
        timestamp_t request_time = mux->buf->s_header.s_local_time;
        enqueue_request(&mux->queue, from, request_time);

        advance_lamport_time();
        ipc_ext_set_status(mux->buf, CS_REPLY, get_lamport_time());
        send(mux->ipcio, from, mux->buf);
      } else if (mux->buf->s_header.s_type == CS_RELEASE) {
        release_request(&mux->queue, from);
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

bool queue_contains(const RequestQueue* q, local_id from) {
  return q->ts[from - 1] != EMPTY_REQUEST_TIME;
}

void enqueue_request(RequestQueue* q, local_id from, timestamp_t t) {
  assert(!queue_contains(q, from));
  q->ts[from - 1] = t;
}

void release_request(RequestQueue* q, local_id from) {
  assert(queue_contains(q, from));
  q->ts[from - 1] = EMPTY_REQUEST_TIME;
}
