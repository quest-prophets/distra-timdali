#include "time.h"

timestamp_t lamport_time = 0;

timestamp_t get_lamport_time() {
  return lamport_time;
}

void advance_lamport_time() {
  lamport_time++;
}

void synchronize_time(timestamp_t received_time) {
  lamport_time = received_time > lamport_time ? received_time : lamport_time;
}
