#pragma once

#include "ipc.h"
#include <stdio.h>

void log_init_events_log();

void log_init_pipes_log();

void log_close_pipes_log();

void log_pipe(local_id dst, local_id from, int read_fd, int write_fd);

void log_event(const char* msg);

void log_eventf(const char* fmt, ...);

void log_panic(local_id pid, const char* error_fmt, ...);
