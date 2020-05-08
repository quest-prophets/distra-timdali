#pragma once

#include "pa2345.h"
#include "ipcio.h"

typedef struct Mutex Mutex;

Mutex* mutex_init(IPCIO* ipcio);
