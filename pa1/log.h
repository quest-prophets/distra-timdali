#pragma once

#include "ipc.h"
#include <stdio.h>

FILE* log_open_events();

FILE* log_open_pipes();

void log_write(FILE* log_file, const char* msg);

void log_print(FILE* log_file, const char* fmt, ...);

void log_panic(local_id pid, const char* error_fmt, ...);
