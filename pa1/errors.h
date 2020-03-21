#pragma once

void panic(local_id pid, const char* error_fmt, ...) {
  va_list args;
  va_start(args, error_fmt);
  fprintf(stderr, "Process %d ", pid);
  vfprintf(stderr, error_fmt, args);
  exit(1);
}
