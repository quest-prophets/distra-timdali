#include "log.h"

#include <stdlib.h>

#include "common.h"

FILE* log_open_events() {
  return fopen(events_log, "a");
}

FILE* log_open_pipes() {
  return fopen(pipes_log, "w+");
}

void log_write(FILE* log_file, const char* msg) {
  printf("%s", msg);
  fputs(msg, log_file);
}

void log_print(FILE* log_file, const char* fmt, ...) {
  char msg[2048];
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg, 2048, fmt, args);
  log_write(log_file, msg);
}

void log_panic(local_id pid, const char* error_fmt, ...) {
  va_list args;
  va_start(args, error_fmt);
  fprintf(stderr, "Process %d ", pid);
  vfprintf(stderr, error_fmt, args);
  exit(1);
}
