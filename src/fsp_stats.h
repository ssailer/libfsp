#pragma once

#include <stddef.h>

typedef struct FSPStats {
  double start_time;
  double log_time;
  double dt_logtime;
  double runtime;

  int n_read_events;
  int n_written_events;
  int n_discarded_events;

  int current_nread;
  int current_nwritten;
  int current_ndiscarded;

  double dt_current;
  double current_read_rate;
  double current_write_rate;
  double current_discard_rate;

  double avg_read_rate;
  double avg_write_rate;
  double avg_discard_rate;

} FSPStats;
