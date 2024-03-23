#ifndef __APPLE__
#define _XOPEN_SOURCE 500
/* needed for random() */
#endif

#include "fsp_processor.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <fsp_channelmaps.h>

/* taken from falcon-daq*/

/*
 * Returns a pseudo-random number which is uniformly distributed in the range
 * of 0 to 1 inclusive. Uses the random(3) API, so use srandom(3) or
 * initstate(3) for seeding the PRNG.
 *
 * Note that this PRNG does not provide cryptographic-quality randomness.
 */
double random_flat(void) { return random() / (double)RAND_MAX; }

/*
 * Returns a pseudo-random number which is exponentially distributed with the
 * given mean. Uses the random(3) API, so use srandom(3) or initstate(3) for
 * seeding the PRNG.
 *
 * Note that this PRNG does not provide cryptographic-quality randomness.
 */
double random_exponential(const double mean) {
  double u;
  while ((u = -random_flat()) == -1.0)
    ;
  return -mean * log1p(u);
}

Timestamp fcio_time_timestamps2run(int timestamps[5]) {
  assert(timestamps);

  Timestamp runtime;
  runtime.seconds = timestamps[1];
  runtime.nanoseconds = 1e9 * (double)timestamps[2] / ((double)timestamps[3] + 1.0);

  return runtime;
}

Timestamp fcio_time_ticks2run(int pps, int ticks, int maxticks) {
  Timestamp runtime;
  runtime.seconds = pps;
  runtime.nanoseconds = 1e9 * (double)ticks / ((double)maxticks + 1.0);

  return runtime;
}

Timestamp fcio_time_run2unix(Timestamp runtime, int timeoffsets[5], int use_external_clock) {
  assert(timeoffsets);

  if (use_external_clock) {
    runtime.seconds += timeoffsets[2];
  } else {
    runtime.seconds += timeoffsets[0];
    runtime.nanoseconds += 1000 * timeoffsets[1];
    while (runtime.nanoseconds >= 1000000000) {
      runtime.seconds++;
      runtime.nanoseconds -= 1000000000;
    }
  }
  return runtime;
}

static inline unsigned int fsp_write_decision(StreamProcessor* processor, FSPState* fsp_state) {
  if ((fsp_state->state->last_tag != FCIOEvent) && (fsp_state->state->last_tag != FCIOSparseEvent))
  // if ((fsp_state->state->last_tag != FCIOEvent))
    return 1;

  if ((fsp_state->flags.event & processor->set_event_flags) ||
      (fsp_state->flags.trigger & processor->set_trigger_flags))
    return 1;

  return 0;
}

static inline FSPFlags fsp_evt_flags(StreamProcessor* processor, FSPFlags flags, FCIOState* state) {
  // FCIOStateReader* reader = processor->buffer->reader;
  /*
  Determine if:
  - pulser event
  - baseline event
  - muon event
  - retrigger event
  */
  if (!state) {
    return flags;
  }

  if ((state->last_tag != FCIOEvent) && (state->last_tag != FCIOSparseEvent)) {
    return flags;
  }

  int nsamples = state->config->eventsamples;
  Timestamp now_ts = fcio_time_timestamps2run(state->event->timestamp);
  unsigned short trace_larger = 0;
  if (processor->aux.pulser_trace_index > -1) {
    if ((trace_larger = fsp_dsp_trace_larger_than(
             state->event->trace[processor->aux.pulser_trace_index], 0, nsamples, nsamples,
             processor->aux.pulser_adc_threshold + state->event->theader[processor->aux.pulser_trace_index][0]))) {
      flags.event |= EVT_AUX_PULSER;
      if (processor->loglevel >= 4)
        fprintf(stderr, "DEBUG fsp_evt_flags: pulser now=%ld.%09ld %u adc\n", now_ts.seconds, now_ts.nanoseconds,
                trace_larger);
    }
  }

  if (processor->aux.baseline_trace_index > -1) {
    if ((trace_larger = fsp_dsp_trace_larger_than(
             state->event->trace[processor->aux.baseline_trace_index], 0, nsamples, nsamples,
             processor->aux.baseline_adc_threshold + state->event->theader[processor->aux.baseline_trace_index][0]))) {
      flags.event |= EVT_AUX_BASELINE;
      if (processor->loglevel >= 4)
        fprintf(stderr, "DEBUG fsp_evt_flags: baseline now=%ld.%09ld %u adc\n", now_ts.seconds, now_ts.nanoseconds,
                trace_larger);
    }
  }

  if (processor->aux.muon_trace_index > -1) {
    if ((trace_larger = fsp_dsp_trace_larger_than(
             state->event->trace[processor->aux.muon_trace_index], 0, nsamples, nsamples,
             processor->aux.muon_adc_threshold + state->event->theader[processor->aux.muon_trace_index][0]))) {
      flags.event |= EVT_AUX_MUON;
      if (processor->loglevel >= 4)
        fprintf(stderr, "DEBUG fsp_evt_flags: muon now=%ld.%09ld %u adc\n", now_ts.seconds, now_ts.nanoseconds,
                trace_larger);
    }
  }

  FSPState* previous_fsp_state = NULL;
  int previous_counter = 0;
  while ((previous_fsp_state = FSPBufferGetState(processor->buffer, previous_counter--))) {
    if (!previous_fsp_state || !previous_fsp_state->in_buffer) break;

    FCIOState* previous_state = previous_fsp_state->state;
    /* TODO Check if Sparseevents retrigger, and how to handle this case*/
    if ((previous_state->last_tag != FCIOEvent) && (previous_state->last_tag != FCIOSparseEvent))
      continue;

    Timestamp previous_ts = fcio_time_timestamps2run(previous_state->event->timestamp);

    Timestamp delta_ts = {.seconds = 0, .nanoseconds = 0};
    if (state->config->adcbits == 16) {
      delta_ts.seconds = 0;
      delta_ts.nanoseconds = state->config->eventsamples * 16;
    } else if (state->config->adcbits == 12) {
      delta_ts.seconds = 0;
      delta_ts.nanoseconds = state->config->eventsamples * 4;
    } else {
      fprintf(stderr, "CRITICAL fsp_evt_flags: Only support 12- or 16-bit ADC data. Got %d bit precision.\n",
              state->config->adcbits);
    }
    Timestamp event_delta = timestamp_sub(now_ts, previous_ts);
    if (timestamp_leq(event_delta, delta_ts)) {
      flags.event |= EVT_RETRIGGER;
      /* Marking the previous event as EVT_EXTENDED happens in UpdateLPPState. */
      if (processor->loglevel >= 4)
        fprintf(stderr, "DEBUG fsp_evt_flags: retrigger now=%ld.%09ld previous=%ld.%09ld delta=%ld.%09ld\n",
                now_ts.seconds, now_ts.nanoseconds, previous_ts.seconds, previous_ts.nanoseconds, delta_ts.seconds,
                delta_ts.nanoseconds);
    }
  }

  return flags;
}

static inline FSPFlags fsp_st_majority(StreamProcessor* processor, FSPFlags flags, FCIOState* state) {
  fcio_config* config = state->config;
  fcio_event* event = state->event;

  fsp_dsp_hardware_majority(processor->hwm_cfg, config->adcs, event->theader);

  /* don't set to higher than 1 if you are sane */
  if (processor->hwm_cfg->multiplicity >= processor->majority_threshold) {
    flags.event |= EVT_FPGA_MULTIPLICITY;

    /* if majority is >= 1, then the following check is safe, otherwise think about what happens when majority == 0
       if there is any channel with a majority above the threshold, it's a force trigger, if not, it should be
       prescaled and not affect the rest of the datastream.
    */
    if (processor->hwm_cfg->n_below_min_value == processor->hwm_cfg->multiplicity)
      flags.event |= EVT_FPGA_MULTIPLICITY_ENERGY_BELOW;
    else
      flags.trigger |= ST_TRIGGER_FORCE;
  }

  return flags;
}

static inline FSPFlags fsp_st_analogue_sum(StreamProcessor* processor, FSPFlags flags, FCIOState* state) {
  fcio_config* config = state->config;
  fcio_event* event = state->event;

  fsp_dsp_windowed_peak_sum(processor->wps_cfg, config->eventsamples, config->adcs, event->trace);

  if (processor->loglevel >= 5) {
    fprintf(stderr, "DEBUG peak_sum_trigger_list evtno=%d,nregions=%d", event->timestamp[0], processor->wps_cfg->trigger_list.size);
    int start = config->eventsamples;
    int stop = 0;
    for (int i = 0; i < processor->wps_cfg->trigger_list.size; i++) {

      if (processor->wps_cfg->trigger_list.start[i] < start)
        start = processor->wps_cfg->trigger_list.start[i];

      if (processor->wps_cfg->trigger_list.stop[i] > stop)
        stop = processor->wps_cfg->trigger_list.stop[i];

      fprintf(stderr, " pe=%.1f,start=%d,stop=%d,size=%d,time_us=%.3f",
        processor->wps_cfg->trigger_list.wps_max[i],
        processor->wps_cfg->trigger_list.start[i],
        processor->wps_cfg->trigger_list.stop[i],
        processor->wps_cfg->trigger_list.stop[i] - processor->wps_cfg->trigger_list.start[i],
        (processor->wps_cfg->trigger_list.stop[i] - processor->wps_cfg->trigger_list.start[i])*16e-3
      );
    }

    // int start = config->eventsamples;
    // int stop = -1;
    // for (int i = 0; i < processor->wps_cfg->trigger_list.size; i++) {

    //   // if (processor->wps_cfg->trigger_list.start[i] > start)
    //   if (processor->wps_cfg->trigger_list.start[i]; <= stop) { // this is the old stop
    //     // overlapping regions (due to coincidence window)
    //     // so we keep the old start and update the new stop
    //     stop = processor->wps_cfg->trigger_list.stop[i];
    //     continue;
    //   } 
    //   start = processor->wps_cfg->trigger_list.start[i];
    //   stop = processor->wps_cfg->trigger_list.stop[i];

      


    //   // if (processor->wps_cfg->trigger_list.stop[i] > stop)


    // }

    fprintf(stderr, " full_region,start=%d,stop=%d,size=%d,time_us=%.3f\n",
      start,stop,stop-start,(stop-start)*16e-3
    );


  }

  if (processor->wps_cfg->max_peak_sum >= processor->sum_threshold_pe) {
    flags.trigger |= ST_WPS_THRESHOLD;
  }

  if (processor->wps_cfg->max_peak_sum >= processor->windowed_sum_threshold_pe) {
    flags.event |= EVT_ASUM_MIN_NPE;
  }

  if (processor->muon_coincidence && flags.event & EVT_AUX_MUON) {
    flags.trigger |= ST_TRIGGER_FORCE;
  }

  return flags;
}

static inline Timestamp generate_prescaling_timestamp(float rate)
{
  double shift = random_exponential(1.0 / rate);
  double integral;
  double fractional = modf(shift, &integral);
  Timestamp timestamp;
  timestamp.seconds = (long)integral;
  timestamp.nanoseconds = (long)(fractional * 1.0e9);
  return timestamp;
}

static inline FSPFlags fsp_st_prescaling(StreamProcessor* processor, FSPFlags flags, Timestamp event_unix_timestamp) {
  /*
    Exit early if:
    - prescaling has not been turned on
    - the event categority contains non-phy triggers / occasions
    - if the software trigger found a force trigger

    -> should result in pure veto triggers being prescaled.
  */

  /* ge prescaling */
  if (processor->ge_prescaling_rate > 0.0 && (flags.event & EVT_FPGA_MULTIPLICITY_ENERGY_BELOW) &&
      ((flags.trigger & ST_TRIGGER_FORCE) == 0)) {
    if (processor->ge_prescaling_timestamp.seconds == -1) {
      /* initialize with the first event in the stream.*/
      processor->ge_prescaling_timestamp = generate_prescaling_timestamp(processor->ge_prescaling_rate);
    }
    else if (timestamp_geq(event_unix_timestamp, processor->ge_prescaling_timestamp)) {
      flags.trigger |= ST_HWM_PRESCALED;
      Timestamp next_timestamp = generate_prescaling_timestamp(processor->ge_prescaling_rate);
      if (processor->loglevel >= 4)
        fprintf(stderr, "DEBUG ge_prescaling current timestamp %ld.%09ld + %ld.%09ld\n",
          processor->ge_prescaling_timestamp.seconds, processor->ge_prescaling_timestamp.nanoseconds,
          next_timestamp.seconds, next_timestamp.nanoseconds
        );
      processor->ge_prescaling_timestamp.seconds += next_timestamp.seconds;
      processor->ge_prescaling_timestamp.nanoseconds += next_timestamp.nanoseconds;
    }
  }

  /* sipm prescaling
    Only prescales events which have not otherwise been triggered.
    Ge prescaling takes precedence.
    TODO: Should this be a combined rate, i.e. the sipm_prescaling_timestamp be shifted even if it was
          a ge_prescaled event?
  */
  if (!processor->sipm_prescaling || flags.trigger) return flags;

  switch (processor->sipm_prescaling[0]) {
    case 'a': {
      if (processor->sipm_prescaling_timestamp.seconds == -1) {
        /* initialize with the first event in the stream.*/
        processor->sipm_prescaling_timestamp = generate_prescaling_timestamp(processor->sipm_prescaling_rate);
      }
      else if (timestamp_geq(event_unix_timestamp, processor->sipm_prescaling_timestamp)) {
        flags.trigger |= ST_WPS_PRESCALED;
        Timestamp next_timestamp = generate_prescaling_timestamp(processor->sipm_prescaling_rate);
        if (processor->loglevel >= 4)
        fprintf(stderr, "DEBUG sipm_prescaling current timestamp %ld.%09ld + %ld.%09ld\n",
          processor->ge_prescaling_timestamp.seconds, processor->ge_prescaling_timestamp.nanoseconds,
          next_timestamp.seconds, next_timestamp.nanoseconds
        );
        processor->sipm_prescaling_timestamp.seconds += next_timestamp.seconds;
        processor->sipm_prescaling_timestamp.nanoseconds += next_timestamp.nanoseconds;
      }

      break;
    }
    case 'o': {
      if (processor->sipm_prescaling_counter == processor->sipm_prescaling_offset) {
        flags.trigger |= ST_WPS_PRESCALED;
        processor->sipm_prescaling_counter = 0;

      } else {
        processor->sipm_prescaling_counter++;
      }

      break;
    }
  }
  return flags;
}

int fsp_process_fcio_state(StreamProcessor* processor, FSPState* fsp_state, FCIOState* state) {
  WindowedPeakSumConfig* wps_cfg = processor->wps_cfg;
  HardwareMajorityConfig* hwm_cfg = processor->hwm_cfg;

  FSPFlags flags = {.event = EVT_NULL, .trigger = ST_NULL};

  fsp_state->stream_tag = state->last_tag;
  fsp_state->flags = flags;
  fsp_state->timestamp.seconds = -1;
  fsp_state->timestamp.nanoseconds = 0;
  fsp_state->unixstamp.seconds = -1;
  fsp_state->unixstamp.nanoseconds = 0;

  switch (state->last_tag) {
    case FCIOSparseEvent:
    case FCIOEvent: {

      fsp_state->timestamp = fcio_time_timestamps2run(state->event->timestamp);
      fsp_state->unixstamp = fcio_time_run2unix(fsp_state->timestamp, state->event->timeoffset, state->config->gps);

      if (processor->checks) {
        const int max_ticks = 249999999;
        if (state->event->timestamp[2] > max_ticks) {
          if (processor->loglevel >= 2)
            fprintf(stderr, "WARNING timestamp of event %i is out of bounds (ticks=%u)\n", state->event->timestamp[0],
                    state->event->timestamp[2]);
        }

        if (state->event->timestamp[3] != max_ticks) {
          if (processor->loglevel >= 2)
            fprintf(stderr, "WARNING lost time synchronisation in previous second (max_ticks=%d)\n",
                    state->event->timestamp[3]);
        }
      }
      flags = fsp_evt_flags(processor, flags, state);

      if (hwm_cfg) {
        flags = fsp_st_majority(processor, flags, state);
        fsp_state->hwm_multiplicity = hwm_cfg->multiplicity;
        fsp_state->hwm_max_value = hwm_cfg->max_value;
        fsp_state->hwm_min_value = hwm_cfg->min_value;
      }

      if (wps_cfg) {
        flags = fsp_st_analogue_sum(processor, flags, state);
        fsp_state->wps_max_value = wps_cfg->max_peak_sum;
        fsp_state->wps_max_sample = wps_cfg->max_peak_sum_at;
        fsp_state->wps_max_single_peak_value = wps_cfg->max_peak;
        fsp_state->wps_multiplicity = wps_cfg->multiplicity;
      }

      flags = fsp_st_prescaling(processor, flags, fsp_state->unixstamp);

      fsp_state->flags = flags;

      fsp_state->contains_timestamp = 1;
      break;
    }

    case FCIORecEvent: {
      fsp_state->timestamp = fcio_time_timestamps2run(state->recevent->timestamp);
      fsp_state->unixstamp = fcio_time_run2unix(fsp_state->timestamp, state->event->timeoffset, state->config->gps);

      fsp_state->contains_timestamp = 1;

      break;
    }

    case FCIOConfig: {
      // format == 0 is already converted to trace indices used by fcio
      if (processor->aux.tracemap_format) {
        if (processor->aux.pulser_trace_index > 0) {
          if (!convert2traceidx(1, &processor->aux.pulser_trace_index, processor->aux.tracemap_format,
                                 state->config->tracemap)) {
            fprintf(stderr, "CRITICAL fsp_process_fcio_state: aux pulser channel could not be mapped.\n");
            return -1;
          }
          if (processor->loglevel >= 4) {
            fprintf(stderr, "DEBUG conversion aux pulser trace index %d\n", processor->aux.pulser_trace_index);
          }
        }
        if (processor->aux.baseline_trace_index > 0) {
          if (!convert2traceidx(1, &processor->aux.baseline_trace_index, processor->aux.tracemap_format,
                                 state->config->tracemap)) {
            fprintf(stderr, "CRITICAL fsp_process_fcio_state: aux baseline channel could not be mapped.\n");
            return -1;
          }
          if (processor->loglevel >= 4) {
            fprintf(stderr, "DEBUG conversion aux baseline trace index %d\n", processor->aux.baseline_trace_index);
          }
        }
        if (processor->aux.muon_trace_index > 0) {
          if (!convert2traceidx(1, &processor->aux.muon_trace_index, processor->aux.tracemap_format,
                                 state->config->tracemap)) {
            fprintf(stderr, "CRITICAL fsp_process_fcio_state: aux muon channel could not be mapped.\n");
            return -1;
          }
          if (processor->loglevel >= 4) {
            fprintf(stderr, "DEBUG conversion aux muon trace index %d\n", processor->aux.muon_trace_index);
          }
        }
      }

      if (processor->wps_cfg) {
        if (processor->wps_cfg->tracemap_format) {
          int success = convert2traceidx(processor->wps_cfg->ntraces, processor->wps_cfg->tracemap,
                                          processor->wps_cfg->tracemap_format, state->config->tracemap);
          if (processor->loglevel >= 4) {
            for (int i = 0; i < processor->wps_cfg->ntraces; i++) {
              fprintf(stderr, "DEBUG conversion analog sum trace index %d\n", processor->wps_cfg->tracemap[i]);
            }
          }
          if (!success) {
            fprintf(stderr,
                    "CRITICAL fsp_process_fcio_state: during conversion of analog sum channels, one channel could not "
                    "be mapped.\n");
            return -1;
          }
        }

        if (wps_cfg->sum_window_stop_sample < 0)
          wps_cfg->sum_window_stop_sample = state->config->eventsamples;

        if (wps_cfg->sum_window_stop_sample + wps_cfg->dsp_post_max_samples >
            state->config->eventsamples) {
          wps_cfg->sum_window_stop_sample =
              state->config->eventsamples - wps_cfg->dsp_post_max_samples;
        }
        if (wps_cfg->sum_window_start_sample - wps_cfg->dsp_pre_max_samples < 0) {
          wps_cfg->sum_window_start_sample = wps_cfg->dsp_pre_max_samples;
        }
        int valid_window = wps_cfg->sum_window_stop_sample - wps_cfg->sum_window_start_sample;
        if (valid_window <= 0) {
          fprintf(stderr,
                  "CRITICAL fsp_process_fcio_state: sum_window_start_sample %d and sum_window_stop_sample %d overlap! "
                  "No samples will be checked.\n",
                  wps_cfg->sum_window_start_sample, wps_cfg->sum_window_stop_sample);
        }
        if (valid_window < wps_cfg->coincidence_window) {
          if (processor->loglevel)
            fprintf(stderr,
                    "ERROR fsp_process_fcio_state: not enough samples for these dsp settings to allow a analog sum "
                    "window of size %d samples, reduced to %d.\n",
                    wps_cfg->coincidence_window, valid_window);
          wps_cfg->coincidence_window = valid_window;
        }

        for (int i = 0; i < wps_cfg->ntraces; i++) {
          wps_cfg->dsp_start_sample[i] =
              wps_cfg->sum_window_start_sample - wps_cfg->dsp_pre_samples[i];
          wps_cfg->dsp_stop_sample[i] =
              wps_cfg->sum_window_stop_sample + wps_cfg->dsp_post_samples[i];
          if (processor->loglevel >= 4) {
            fprintf(stderr,
                    "DEBUG fsp_process_fcio_state: adjusting windows: channel %d sum start %d sum stop %d dsp start %d "
                    "dsp stop %d\n",
                    i, wps_cfg->sum_window_start_sample, wps_cfg->sum_window_stop_sample,
                    wps_cfg->dsp_start_sample[i], wps_cfg->dsp_stop_sample[i]);
          }
        }
      }

      if (processor->hwm_cfg) {
        if (processor->hwm_cfg->tracemap_format) {
          int success = convert2traceidx(processor->hwm_cfg->ntraces, processor->hwm_cfg->tracemap,
                                          processor->hwm_cfg->tracemap_format, state->config->tracemap);
          if (processor->loglevel >= 4) {
            for (int i = 0; i < processor->hwm_cfg->ntraces; i++) {
              fprintf(stderr, "DEBUG conversion fpga majority trace index %d\n",
                      processor->hwm_cfg->tracemap[i]);
            }
          }
          if (!success) {
            fprintf(stderr,
                    "CRITICAL fsp_process_fcio_state: during conversion of fpga majority channels, one channel could "
                    "not be mapped.\n");
            return -1;
          }
        }

        if (hwm_cfg->ntraces <= 0) {
          hwm_cfg->ntraces = state->config->adcs;
          processor->majority_threshold = 0;  // we pass the event even if there is no fpga_energy; it must be in the stream for a reason.
        }
      }
      fsp_state->contains_timestamp = 0;

      break;
    }
    case FCIOStatus:
      // TODO check if the the data[0].pps/ticks/maxticks is a valid timestamp for the statuspaket
      // the status.statustime throws away the nanoseconds. The only advantage would be, that we might
      // send some soft triggered events a bit earlier, as the pre-trigger timestamp shift, if we use the
      // status timestamp here.
      // fsp_state->timestamp = fcio_time_ticks2run(fsp_state->state->status->statustime); break;
    default: {
      fsp_state->contains_timestamp = 0;
      break;
    }
  }

  return fsp_state->contains_timestamp;
}

void fsp_process_timings(StreamProcessor* processor, FSPState* fsp_state) {

  if (timestamp_geq(processor->post_trigger_timestamp, fsp_state->timestamp)) {
    fsp_state->flags.event |= EVT_FORCE_POST_WINDOW;

    if (fsp_state->flags.event & EVT_ASUM_MIN_NPE) {
      fsp_state->flags.trigger |= ST_WPS_RELTRIGGER;
    }
  }

  if (fsp_state->flags.trigger & ST_TRIGGER_FORCE) {
    /* current state is germanium trigger, keep it and start checking all previous and future states against
      the trigger windows.
    */
    processor->force_trigger_timestamp = fsp_state->timestamp;
    processor->post_trigger_timestamp =
        timestamp_add(processor->force_trigger_timestamp, processor->post_trigger_window);
    processor->pre_trigger_timestamp = timestamp_sub(processor->force_trigger_timestamp, processor->pre_trigger_window);

    FSPState* update_fsp_state = NULL;
    int previous_counter = 0;  // current fsp_state is a peeked state, so GetState(0) is the "previous one"
    while ((update_fsp_state = FSPBufferGetState(processor->buffer, previous_counter--))
            && timestamp_geq(update_fsp_state->timestamp, processor->pre_trigger_timestamp))
    {
      if (!update_fsp_state->contains_timestamp)
        continue;
      update_fsp_state->flags.event |= EVT_FORCE_PRE_WINDOW;
      if (update_fsp_state->flags.event & EVT_ASUM_MIN_NPE) {
        update_fsp_state->flags.trigger |= ST_WPS_RELTRIGGER;
      }
    }
  }

  if (fsp_state->flags.event & EVT_RETRIGGER) {
    FSPState* update_fsp_state = NULL;
    int previous_counter = 0;
    while ( (update_fsp_state = FSPBufferGetState(processor->buffer, previous_counter--)) ) {
      if (!update_fsp_state->contains_timestamp)
        continue;
      if ((update_fsp_state->flags.event & EVT_RETRIGGER) == 0) {
        update_fsp_state->flags.event |= EVT_EXTENDED;
        break;
      }
    }
  }
}

int fsp_process(StreamProcessor* processor, FSPState* fsp_state, FCIOState* state) {
  fsp_state->state = state;

  int rc = fsp_process_fcio_state(processor, fsp_state, state);
  if (rc == 1)
    fsp_process_timings(processor, fsp_state);
  if (rc == -1)
    return 0;
  return 1;
}

int FSPInput(StreamProcessor* processor, FCIOState* state) {
  if (!processor || !state) return 0;

  FSPState* fsp_state = FSPBufferPeekState(processor->buffer);

  if (!fsp_state) {
    fprintf(stderr, "CRITICAL LPPInput: Buffer full, refuse to overwrite.\n");
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
               // work out, and the LPPGetNextState function returns NULL on nfree = 0;

  return FSPBufferFreeLevel(processor->buffer);
}

FSPState* FSPOutput(StreamProcessor* processor) {
  if (!processor) return NULL;

  FSPState* fsp_state = FSPBufferFetchState(processor->buffer);

  if (!fsp_state) {
    return NULL;
  }

  fsp_state->write = fsp_write_decision(processor, fsp_state);

  if (processor->wps_cfg)
    fsp_state->wps_trigger_list = &processor->wps_cfg->trigger_list;
  else
    fsp_state->wps_trigger_list = NULL;

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

void FSPEnableTriggerFlags(StreamProcessor* processor, unsigned int flags) {
  processor->set_trigger_flags = flags;
  if (processor->loglevel >= 4) fprintf(stderr, "DEBUG LPPEnableTriggerFlags: %u\n", flags);
}

void FSPEnableEventFlags(StreamProcessor* processor, unsigned int flags) {
  processor->set_event_flags = flags;
  if (processor->loglevel >= 4) fprintf(stderr, "DEBUG LPPEnableEventFlags: %u\n", flags);
}

StreamProcessor* FSPCreate(void) {
  StreamProcessor* processor = calloc(1, sizeof(StreamProcessor));

  processor->stats = calloc(1, sizeof(FSPStats));

  processor->minimum_buffer_window.seconds = 0;
  processor->minimum_buffer_window.nanoseconds =
      (FCIOMaxSamples + 1) * 16;        // this is required to check for retrigger events
  processor->minimum_buffer_depth = 16; // the minimum buffer window * 30kHz event rate requires at least 16 records
  processor->stats->start_time = 0.0;    // reset, actual start time happens with the first record insertion.
  processor->ge_prescaling_timestamp.seconds = -1; // will init when it's needed
  processor->sipm_prescaling_timestamp.seconds = -1; // will init when it's needed

  /* default tracemap for HW and PS are fine, as they are allocated to zero.
     special aux channels need to be below zero, as they don't have an ntraces counter.
  */
  processor->aux.pulser_trace_index = -1;
  processor->aux.baseline_trace_index = -1;
  processor->aux.muon_trace_index = -1;

  /* hardcoded defaults which should make sense. Used SetFunctions outside to overwrite */
  FSPEnableEventFlags(processor, EVT_AUX_PULSER | EVT_AUX_BASELINE | EVT_EXTENDED | EVT_RETRIGGER);
  FSPEnableTriggerFlags(processor, ST_TRIGGER_FORCE | ST_WPS_RELTRIGGER | ST_WPS_THRESHOLD |
                                       ST_WPS_PRESCALED | ST_HWM_PRESCALED);

  return processor;
}

void FSPSetLogLevel(StreamProcessor* processor, int loglevel) { processor->loglevel = loglevel; }

void FSPSetLogTime(StreamProcessor* processor, double log_time) { processor->stats->log_time = log_time; }

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
    if (processor->loglevel) fprintf(stderr, "ERROR LPPSetBufferSize: Couldn't allocate LPPBuffer.\n");

    return 0;
  }
    if (processor->loglevel >=2) {
        fprintf(stderr, "DEBUG LPPSetBufferSize to depth %d and window %ld.%09ld\n", processor->buffer->max_states, processor->buffer->buffer_window.seconds, processor->buffer->buffer_window.nanoseconds);
    }
  return buffer_depth;
}

void FSPDestroy(StreamProcessor* processor) {
  FSPBufferDestroy(processor->buffer);
  free(processor->stats);
  free(processor->hwm_cfg);
  free(processor->wps_cfg);
  free(processor);
}

int FSPFlush(StreamProcessor* processor) {
  if (!processor) return 0;

  return FSPBufferFlush(processor->buffer);
}

int LPPFreeStates(StreamProcessor* processor) {
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
  int nfree = LPPFreeStates(processor);

  while (!(fsp_state = FSPOutput(processor))) {
    /* this should handle the timeout, so we don't have to do it in the postprocessor.
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
      // int n_free_buffer_states = LPPInput(processor, state); // got a valid state, process and hope that we get a new
      // fsp_state
      nfree = FSPInput(processor,
                       state);  // got a valid state, process and hope that we get a new fsp_state on the next loop
    }
  }

  return fsp_state;
}
