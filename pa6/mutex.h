#pragma once

#include "pa2345.h"
#include "ipcio.h"

typedef struct Mutex Mutex;

#define NON_CS_MESSAGE_RECEIVED 1

Mutex* mutex_init(IPCIO* ipcio, Message* buf, local_id num_children);

int handle_cs_requests(Mutex* mux);
