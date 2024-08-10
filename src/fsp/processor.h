#pragma once

#include "timestamps.h"
#include "flags.h"
#include "stats.h"
#include "buffer.h"
#include "dsp.h"

#define FC_MAXTICKS 249999999

typedef struct {

  int hwm_threshold;
  int hwm_prescale_ratio;
  int wps_prescale_ratio;
  int muon_coincidence;

  float relative_wps_threshold;
  float absolute_wps_threshold;
  float wps_prescale_rate;
  float hwm_prescale_rate;

  HWMFlags wps_reference_flags_hwm;
  CTFlags wps_reference_flags_ct;
  WPSFlags wps_reference_flags_wps;

  FSPWriteFlags enabled_flags;
  Timestamp pre_trigger_window;
  Timestamp post_trigger_window;

} FSPConfig;

typedef struct StreamProcessor {

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

  int wps_prescale_ready_counter;
  Timestamp wps_prescale_timestamp;

  int hwm_prescale_ready_counter;
  Timestamp hwm_prescale_timestamp;

  int loglevel;
  int checks;

  FSPConfig config;

  Timestamp minimum_buffer_window;
  unsigned int minimum_buffer_depth;
  FSPBuffer *buffer;

  DSPWindowedPeakSum *dsp_wps;
  DSPHardwareMajority *dsp_hwm;
  DSPChannelThreshold *dsp_ct;
  FSPStats *stats;

} StreamProcessor;

/* Con-/Destructors and required setup. */

StreamProcessor *FSPCreate(unsigned int buffer_depth);
void FSPDestroy(StreamProcessor *processor);
int FSPSetBufferSize(StreamProcessor *processor, unsigned int buffer_depth);

/* Change defaults*/

void FSPSetLogLevel(StreamProcessor *processor, int loglevel);
void FSPSetLogTime(StreamProcessor *processor, double log_time);

void FSPEnableTriggerFlags(StreamProcessor *processor, STFlags flags);
void FSPEnableEventFlags(StreamProcessor *processor, EventFlags flags);
void FSPSetWPSReferenceFlag(StreamProcessor* processor, uint64_t hwm_flags, uint64_t ct_flags, uint64_t wps_flags);

/* Use FSPGetNextState to process states provided by FCIOStateReader until it returns NULL.
    - Feed FCIOStates from FCIOGetNextStatevia FSPInput
    - Poll FSPOutput until NULL
    - if states are null, buffer is flushed
*/
FSPState *FSPGetNextState(StreamProcessor *processor, FCIOStateReader *reader, int *timedout);

int FSPInput(StreamProcessor *processor, FCIOState *state);
FSPState *FSPOutput(StreamProcessor *processor);
int FSPFlush(StreamProcessor *processor);
int FSPFreeStates(StreamProcessor *processor);

int FSPStatsInfluxString(StreamProcessor* processor, char* logstring, size_t logstring_size);
int FSPStatsUpdate(StreamProcessor* processor, int force);
