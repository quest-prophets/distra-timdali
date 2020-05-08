#include "mutex.h"
#include "time.h"

#include <stdlib.h>

/* Request queue */

#define QUEUE_LEN MAX_CHILDREN

typedef struct EnqueuedRequest {
  timestamp_t recv_t;
  local_id process_id;
} EnqueuedRequest;

typedef struct {
  EnqueuedRequest items[QUEUE_LEN];
} RequestQueue;

EnqueuedRequest top_enqueued_request(const RequestQueue* q);

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

// Request queue is sorted by timestamp -- a tuple of (recv_t, process_id)
uint32_t request_timestamp(EnqueuedRequest request) {
  return (request.recv_t << 16) | request.process_id;
}

#define EMPTY_REQUEST ((EnqueuedRequest){0, 0})
#define EMPTY_TIMESTAMP request_timestamp(EMPTY_REQUEST)

EnqueuedRequest top_enqueued_request(const RequestQueue* q) {
  EnqueuedRequest head = EMPTY_REQUEST;
  for (int i = 0; i < QUEUE_LEN; ++i) {
    if (request_timestamp(q->items[i]) > request_timestamp(head))
      head = q->items[i];
  }
  return head;
}

void enqueue_request(RequestQueue* q, local_id from, timestamp_t t) {
  for (int i = 0; i < QUEUE_LEN; ++i) {
    if (request_timestamp(q->items[i]) == EMPTY_TIMESTAMP) {
      q->items[i] = (EnqueuedRequest){.process_id = from, .recv_t = t};
      break;
    }
  }
}

void release_request(RequestQueue* q, local_id from) {
  for (int i = 0; i < QUEUE_LEN; ++i) {
    if (q->items[i].process_id == from) {
      q->items[i] = EMPTY_REQUEST;
      break;
    }
  }
}
