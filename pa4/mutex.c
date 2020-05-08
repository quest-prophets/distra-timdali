#include "mutex.h"
#include "time.h"

#include <assert.h>
#include <stdlib.h>

/* Request queue */

/* The queue is based on the follwing assumptions:
 * - each process can send at most once request for the CS before releasing it
 * - lamport clock starts at 1, so 0 can be reserved to indicate "no requests"
 * - there's always at least one (own) request in the queue
 */

typedef struct {
  timestamp_t ts[MAX_CHILDREN];
} RequestQueue;

local_id top_enqueued_request(const RequestQueue* q);

void enqueue_request(RequestQueue* q, local_id from, timestamp_t t);

void release_request(RequestQueue* q, local_id from);

/* Mutual exclusion */

struct Mutex {
  IPCIO* ipcio;
  RequestQueue queue;
};

int request_cs(const void* Mutex) {
  // ...
}

int release_cs(const void* Mutex) {
  // ...
}

Mutex* mutex_init(IPCIO* ipcio) {
  Mutex* mutex = (Mutex*)calloc(1, sizeof(Mutex));
  mutex->ipcio = ipcio;
  return mutex;
}

/* Request queue */

#define EMPTY_REQUEST_TIME 0

local_id top_enqueued_request(const RequestQueue* q) {
  local_id top_enqueued = 0;
  timestamp_t top_enqueued_ts = q->ts[0];
  for (int i = 0; i < MAX_CHILDREN; ++i) {
    if (q->ts[i] > top_enqueued_ts) {
      top_enqueued = i;
      top_enqueued_ts = q->ts[i];
    }
  }
  return top_enqueued;
}

void enqueue_request(RequestQueue* q, local_id from, timestamp_t t) {
  assert(q->ts[from] == EMPTY_REQUEST_TIME);
  q->ts[from] = t;
}

void release_request(RequestQueue* q, local_id from) {
  assert(q->ts[from] != EMPTY_REQUEST_TIME);
  q->ts[from] = EMPTY_REQUEST_TIME;
}
