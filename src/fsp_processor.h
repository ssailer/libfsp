#pragma once

#include <stddef.h>
#include <stdio.h>

#include <fsp_dsp.h>
#include <fsp_buffer.h>
#include <fsp_state.h>
#include <fsp_timestamps.h>
#include <fsp_stats.h>

#include <fcio.h>

typedef struct StreamProcessor {
  Timestamp pre_trigger_window;
  Timestamp post_trigger_window;

  int nrecords_read;
  int nrecords_written;
  int nrecords_discarded;

  int nevents_read;
  int nevents_written;
  int nevents_discarded;

  /* move to processor */
  Timestamp force_trigger_timestamp;
  Timestamp post_trigger_timestamp;
  Timestamp pre_trigger_timestamp;

  float relative_sum_threshold_pe;
  float absolute_sum_threshold_pe;
  int majority_threshold;
  int muon_coincidence;

  int sipm_prescaling_offset;
  int sipm_prescaling_counter;
  float sipm_prescaling_rate;
  char *sipm_prescaling;

  Timestamp sipm_prescaling_timestamp;
  Timestamp ge_prescaling_timestamp;

  int ge_prescaling_threshold_adc;
  float ge_prescaling_rate;

  int loglevel;

  FSPStats* stats;

  int checks;

  unsigned int set_trigger_flags;
  unsigned int set_event_flags;

  struct {
    int pulser_trace_index;
    int pulser_adc_threshold;

    int baseline_trace_index;
    int baseline_adc_threshold;

    int muon_trace_index;
    int muon_adc_threshold;

    int tracemap_format;
  } aux;

  FSPBuffer *buffer;
  Timestamp minimum_buffer_window;
  int minimum_buffer_depth;

  WindowedPeakSumConfig *wps_cfg;
  HardwareMajorityConfig *hwm_cfg;

} StreamProcessor;

/* Con-/Destructors and required setup. */

StreamProcessor *FSPCreate(void);
void FSPDestroy(StreamProcessor *processor);
int FSPSetBufferSize(StreamProcessor *processor, int buffer_depth);

/* Change defaults*/

void FSPSetLogLevel(StreamProcessor *processor, int loglevel);
void FSPSetLogTime(StreamProcessor *processor, double log_time);

void FSPEnableTriggerFlags(StreamProcessor *processor, unsigned int flags);
void FSPEnableEventFlags(StreamProcessor *processor, unsigned int flags);

/* use in loop operations:
  - Feed FCIOStates via LPPInput asap
  - Poll LPPOutput until NULL
  - if states are null, buffer is flushed
*/



int FSPInput(StreamProcessor *processor, FCIOState *state);
FSPState *FSPOutput(StreamProcessor *processor);
int FSPFlush(StreamProcessor *processor);
int LPPFreeStates(StreamProcessor *processor);
FSPState *FSPGetNextState(StreamProcessor *processor, FCIOStateReader *reader, int *timedout);

int FSPStatsInfluxString(StreamProcessor* processor, char* logstring, size_t logstring_size);
int FSPStatsUpdate(StreamProcessor* processor, int force);
