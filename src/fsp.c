#include "fsp.h"

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

#include <fsp_processor.h>
#include <fcio.h>
#include <time_utils.h>

int FSPInput(StreamProcessor* processor, FCIOState* state) {
  if (!processor || !state) return 0;

  FSPState* fsp_state = FSPBufferPeekState(processor->buffer);

  if (!fsp_state) {
    fprintf(stderr, "CRITICAL FSPInput: Buffer full, refuse to overwrite.\n");
    return 0;
  }
  if ((state->last_tag == FCIOEvent) || (state->last_tag == FCIOSparseEvent)) {
    processor->nevents_read++;
  }
  processor->nrecords_read++;

  int rc = fsp_process(processor, fsp_state, state);

  FSPBufferCommitState(processor->buffer);

  if (!rc)
    return 0;  // This is a proxy for 0 free states, even though we do have some. Something in previous code did not
               // work out, and the FSPGetNextState function returns NULL on nfree = 0;

  return FSPBufferFreeLevel(processor->buffer);
}


FSPState* FSPOutput(StreamProcessor* processor) {
  if (!processor) return NULL;

  FSPState* fsp_state = FSPBufferFetchState(processor->buffer);

  if (!fsp_state) {
    return NULL;
  }

  fsp_state->write = fsp_write_decision(fsp_state);

  // if (processor->wps_cfg)
  //   fsp_state->obs.wps.trigger_list = &processor->wps_cfg->trigger_list;
  // else
  //   fsp_state->obs.wps.trigger_list = NULL;

  if (fsp_state->write) {
    processor->nrecords_written++;
    if ((fsp_state->state->last_tag == FCIOEvent) || (fsp_state->state->last_tag == FCIOSparseEvent))
      processor->nevents_written++;

  } else {
    processor->nrecords_discarded++;
    if ((fsp_state->state->last_tag == FCIOEvent) || (fsp_state->state->last_tag == FCIOSparseEvent))
      processor->nevents_discarded++;
  }

  return fsp_state;
}

void FSPEnableTriggerFlags(StreamProcessor* processor, STFlags flags) {
  processor->enabled_flags.trigger = flags;
  if (processor->loglevel >= 4) fprintf(stderr, "DEBUG FSPEnableTriggerFlags: %lu\n", flags.is_flagged);
}

void FSPEnableEventFlags(StreamProcessor* processor, EventFlags flags) {
  processor->enabled_flags.event = flags;
  if (processor->loglevel >= 4) fprintf(stderr, "DEBUG FSPEnableEventFlags: %lu\n", flags.is_flagged);
}

void FSPSetWPSReferenceFlag(StreamProcessor* processor, uint64_t hwm_flags, uint64_t ct_flags, uint64_t wps_flags) {
  processor->wps_reference_flags_ct = ct_flags;
  processor->wps_reference_flags_hwm = hwm_flags;
  processor->wps_reference_flags_wps = wps_flags;
  if (processor->loglevel >= 4) fprintf(stderr, "DEBUG FSPSetWPSReferenceFlags: hwm %lu ct %lu wps %lu\n", hwm_flags, ct_flags, wps_flags);
}

StreamProcessor* FSPCreate(void) {
  StreamProcessor* processor = calloc(1, sizeof(StreamProcessor));

  processor->stats = calloc(1, sizeof(FSPStats));

  processor->minimum_buffer_window.seconds = 0;
  processor->minimum_buffer_window.nanoseconds =
      (FCIOMaxSamples + 1) * 16;        // this is required to check for retrigger events
  processor->minimum_buffer_depth = 16; // the minimum buffer window * 30kHz event rate requires at least 16 records
  processor->stats->start_time = 0.0;    // reset, actual start time happens with the first record insertion.
  processor->hwm_prescaling_timestamp.seconds = -1; // will init when it's needed
  processor->wps_prescaling_timestamp.seconds = -1; // will init when it's needed

  /* hardcoded defaults which should make sense. Used SetFunctions outside to overwrite */
  // FSPEnableEventFlags(processor, EVT_EXTENDED | EVT_RETRIGGER);
  // FSPEnableTriggerFlags(processor, ST_HWM_TRIGGER | ST_HWM_PRESCALED | ST_WPS_ABS_TRIGGER | ST_WPS_REL_TRIGGER | ST_WPS_PRESCALED | ST_CT_TRIGGER );
  // FSPSetWPSReferenceFlag(processor, EVT_HWM_MULT_THRESHOLD);
  FSPEnableEventFlags(processor, (EventFlags){ .is_retrigger = 1, .is_extended = 1});
  FSPEnableTriggerFlags(processor, (STFlags){ .hwm_multiplicity = 1, .hwm_prescaled = 1, .wps_abs = 1, .wps_rel = 1, .wps_prescaled = 1, .ct_multiplicity = 1} );
  HWMFlags ref_hwm = {0};
  ref_hwm.multiplicity_threshold = 1;
  CTFlags ref_ct = {0};
  WPSFlags ref_wps = {0};
  FSPSetWPSReferenceFlag(processor, ref_hwm.is_flagged, ref_ct.is_flagged, ref_wps.is_flagged);

  return processor;
}

void FSPSetLogLevel(StreamProcessor* processor, int loglevel) {
  processor->loglevel = loglevel;
}

void FSPSetLogTime(StreamProcessor* processor, double log_time) {
  processor->stats->log_time = log_time;
}

int FSPSetBufferSize(StreamProcessor* processor, int buffer_depth) {
  if (processor->buffer) {
    FSPBufferDestroy(processor->buffer);
  }
  if (buffer_depth < processor->minimum_buffer_depth) buffer_depth = processor->minimum_buffer_depth;

  Timestamp buffer_window = timestamp_greater(processor->minimum_buffer_window, processor->pre_trigger_window)
                                ? processor->minimum_buffer_window
                                : processor->pre_trigger_window;
  processor->buffer = FSPBufferCreate(buffer_depth, buffer_window);
  if (!processor->buffer) {
    if (processor->loglevel) fprintf(stderr, "ERROR FSPSetBufferSize: Couldn't allocate FSPBuffer.\n");

    return 0;
  }
    if (processor->loglevel >=2) {
        fprintf(stderr, "DEBUG FSPSetBufferSize to depth %d and window %ld.%09ld\n", processor->buffer->max_states, processor->buffer->buffer_window.seconds, processor->buffer->buffer_window.nanoseconds);
    }
  return buffer_depth;
}

void FSPDestroy(StreamProcessor* processor) {
  FSPBufferDestroy(processor->buffer);
  free(processor->stats);
  free(processor->hwm_cfg);
  free(processor->wps_cfg);
  free(processor->ct_cfg);
  free(processor);
}

int FSPFlush(StreamProcessor* processor) {
  if (!processor) return 0;

  return FSPBufferFlush(processor->buffer);
}

int FSPFreeStates(StreamProcessor* processor) {
  if (!processor) return 0;

  return processor->buffer->max_states - processor->buffer->fill_level;
}

static inline void fsp_init_stats(StreamProcessor* processor) {
  FSPStats* stats = processor->stats;
  if (stats->start_time == 0.0) {
    stats->dt_logtime = stats->start_time = elapsed_time(0.0);
  }
}

FSPState* FSPGetNextState(StreamProcessor* processor, FCIOStateReader* reader, int* timedout) {
  /*
  - check if buffered -> write
  - no buffered, getnextstate until buffered reader -> write
  - no buffered and getnextstate null -> exit
  */
  if (timedout) *timedout = 0;

  if (!processor || !reader) return NULL;

  if (!processor->buffer && FSPSetBufferSize(processor, reader->max_states - 1)) return NULL;

  fsp_init_stats(processor);

  FSPState* fsp_state;
  FCIOState* state;
  int nfree = FSPFreeStates(processor);

  while (!(fsp_state = FSPOutput(processor))) {
    /* this should handle the timeout, so we don't have to do it in the processor.
     */
    if (!nfree) {
      if (timedout) *timedout = 10;
      return NULL;
    }
    state = FCIOGetNextState(reader, timedout);

    if (!state) {
      /* End-of-Stream or timeout reached, we assume finish and flush
        timedout == 0 here means stream is closed, no timeout was reached
        timedout == 1 no event from buffer for specified timeout interval.
        timedout == 2 stream is still alive, but only unselected tags arrived
      */
      if (FSPFlush(processor)) {
        continue;  // still something in the buffer, try to read the rest after unlocking the buffer window.
      } else {
        return NULL;
      }
    } else {
      // int n_free_buffer_states = FSPInput(processor, state); // got a valid state, process and hope that we get a new
      // fsp_state
      nfree = FSPInput(processor,
                       state);  // got a valid state, process and hope that we get a new fsp_state on the next loop
    }
  }

  return fsp_state;
}
