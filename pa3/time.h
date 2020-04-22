#pragma once

#include "banking.h"

void advance_lamport_time();

void synchronize_time(timestamp_t received_time);
