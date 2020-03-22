#include "log.h"

#include <assert.h>
#include <stdlib.h>

#include "common.h"

FILE* events_log_file;
FILE* pipes_log_file;

void log_init_events_log() {
  events_log_file = fopen(events_log, "a");
}

void log_init_pipes_log() {
  pipes_log_file = fopen(pipes_log, "w+");
}

void log_close_pipes_log() {
  assert(pipes_log_file != NULL);

  fclose(pipes_log_file);
}

void log_pipe(local_id dst, local_id from, int read_fd, int write_fd) {
  assert(pipes_log_file != NULL);

  fprintf(pipes_log_file, "%d <- %d: read %d, write %d\n", dst, from, read_fd,
          write_fd);
}

void log_event(const char* msg) {
  assert(events_log_file != NULL);

  printf("%s", msg);
  fputs(msg, events_log_file);
}

void log_eventf(const char* fmt, ...) {
  assert(events_log_file != NULL);

  va_list printf_args, fprintf_args;
  va_start(printf_args, fmt);
  va_copy(fprintf_args, printf_args);

  vprintf(fmt, printf_args);
  vfprintf(events_log_file, fmt, fprintf_args);

  va_end(printf_args);
  va_end(fprintf_args);
}

void log_panic(local_id pid, const char* error_fmt, ...) {
  va_list args;
  va_start(args, error_fmt);
  fprintf(stderr, "Process %d ", pid);
  vfprintf(stderr, error_fmt, args);
  exit(1);
}
