#pragma once

#include "ipc.h"

timestamp_t get_lamport_time();

void advance_lamport_time();

void synchronize_time(timestamp_t received_time);
