#include "fsp_stats.h"

#include <stdio.h>

#include <fsp_processor.h>
#include <time_utils.h>

int FSPStatsUpdate(StreamProcessor* processor, int force) {
  FSPStats* stats = processor->stats;

  if (elapsed_time(stats->dt_logtime) > stats->log_time || force) {
    stats->runtime = elapsed_time(stats->start_time);
    stats->dt_current = elapsed_time(stats->dt_logtime);
    stats->dt_logtime = elapsed_time(0.0);

    stats->current_nread = processor->nevents_read - stats->n_read_events;
    stats->n_read_events = processor->nevents_read;

    stats->current_nwritten = processor->nevents_written - stats->n_written_events;
    stats->n_written_events = processor->nevents_written;

    stats->current_ndiscarded = processor->nevents_discarded - stats->n_discarded_events;
    stats->n_discarded_events = processor->nevents_discarded;

    stats->current_read_rate = stats->current_nread / stats->dt_current;
    stats->current_write_rate = stats->current_nwritten / stats->dt_current;
    stats->current_discard_rate = stats->current_ndiscarded / stats->dt_current;

    stats->avg_read_rate = stats->n_read_events / stats->runtime;
    stats->avg_write_rate = stats->n_written_events / stats->runtime;
    stats->avg_discard_rate = stats->n_discarded_events / stats->runtime;

    return 1;
  }
  return 0;
}

int FSPStatsInfluxString(StreamProcessor* processor, char* logstring, size_t logstring_size) {

  FSPStats* stats = processor->stats;

  int ret = snprintf(logstring, logstring_size,
                     "run_time=%.03f,cur_read=%.03f,cur_write=%.03f,cur_discard=%.03f,avg_read=%.03f,avg_write=%.03f,"
                     "avg_discard=%.03f,cur_nread=%d,cur_nsent=%d,total_nread=%d,total_nsent=%d,total_ndiscarded=%d",
                     stats->runtime, stats->current_read_rate, stats->current_write_rate, stats->current_discard_rate,
                     stats->avg_read_rate, stats->avg_write_rate, stats->avg_discard_rate, stats->current_nread,
                     stats->current_nwritten, stats->n_read_events, stats->n_written_events, stats->n_discarded_events);

  if (ret >= 0 && ret < (int)logstring_size) return 1;
  return 0;
}