#pragma once

#include <string.h>

#include <fsp.h>
#include <fcio.h>
#include "fsp/stats.h"

static inline void write_sequence(char* data, size_t size) {
  for (size_t i = 0; i < size; i++) {
    *(data+i) = i;
  }
}

void fill_default_fspconfig(StreamProcessor* proc)
{
  write_sequence((char*)&proc->config, sizeof(proc->config));

  proc->buffer->max_states = 100;
  write_sequence((char*)&proc->buffer->buffer_window, sizeof(proc->buffer->buffer_window));

  proc->dsp_hwm->ntraces = FCIOMaxChannels;
  write_sequence((char*)&proc->dsp_hwm->tracemap, sizeof(proc->dsp_hwm->tracemap));
  write_sequence((char*)&proc->dsp_hwm->fpga_energy_threshold_adc, sizeof(proc->dsp_hwm->fpga_energy_threshold_adc));

  proc->dsp_ct->ntraces = FCIOMaxChannels;
  write_sequence((char*)&proc->dsp_ct->tracemap, sizeof(proc->dsp_ct->tracemap));
  write_sequence((char*)&proc->dsp_ct->thresholds, sizeof(proc->dsp_ct->thresholds));

  proc->dsp_wps->ntraces = FCIOMaxChannels;

  proc->dsp_wps->apply_gain_scaling = 1;
  proc->dsp_wps->coincidence_window = FCIOMaxSamples/2;
  proc->dsp_wps->sum_window_start_sample = 0;
  proc->dsp_wps->sum_window_stop_sample = FCIOMaxSamples;
  proc->dsp_wps->coincidence_threshold = 10.0;

  write_sequence((char*)&proc->dsp_wps->tracemap, sizeof(proc->dsp_wps->tracemap));
  write_sequence((char*)&proc->dsp_wps->gains, sizeof(proc->dsp_wps->gains));
  write_sequence((char*)&proc->dsp_wps->thresholds, sizeof(proc->dsp_wps->thresholds));
  write_sequence((char*)&proc->dsp_wps->lowpass, sizeof(proc->dsp_wps->lowpass));
  write_sequence((char*)&proc->dsp_wps->shaping_widths, sizeof(proc->dsp_wps->shaping_widths));

  proc->dsp_wps->dsp_pre_max_samples = 3;
  proc->dsp_wps->dsp_post_max_samples = 3;

  write_sequence((char*)&proc->dsp_wps->dsp_pre_samples, sizeof(proc->dsp_wps->dsp_pre_samples));
  write_sequence((char*)&proc->dsp_wps->dsp_post_samples, sizeof(proc->dsp_wps->dsp_post_samples));
  write_sequence((char*)&proc->dsp_wps->dsp_start_sample, sizeof(proc->dsp_wps->dsp_start_sample));
  write_sequence((char*)&proc->dsp_wps->dsp_stop_sample, sizeof(proc->dsp_wps->dsp_stop_sample));

}

void fill_default_fspevent(FSPState* state)
{
  write_sequence((char*)&state->write_flags, sizeof(state->write_flags));
  write_sequence((char*)&state->proc_flags, sizeof(state->proc_flags));
  write_sequence((char*)&state->obs.evt, sizeof(state->obs.evt));
  write_sequence((char*)&state->obs.hwm, sizeof(state->obs.hwm));
  write_sequence((char*)&state->obs.wps, sizeof(state->obs.wps));

  state->obs.ct.multiplicity = FCIOMaxChannels;
  write_sequence((char*)&state->obs.ct.trace_idx, sizeof(state->obs.ct.trace_idx));
  write_sequence((char*)&state->obs.ct.max, sizeof(state->obs.ct.max));

  state->obs.sub_event_list.size = FCIOMaxSamples;
  write_sequence((char*)&state->obs.sub_event_list.start, sizeof(state->obs.sub_event_list.start));
  write_sequence((char*)&state->obs.sub_event_list.stop, sizeof(state->obs.sub_event_list.stop));
  write_sequence((char*)&state->obs.sub_event_list.wps_max, sizeof(state->obs.sub_event_list.wps_max));

}

void fill_default_fspstatus(StreamProcessor* proc)
{
  write_sequence((char*)proc->stats, sizeof(FSPStats));
}


int is_same_fspstatus(StreamProcessor *left, StreamProcessor *right)
{
  return 0 == memcmp(left->stats, right->stats, sizeof(FSPStats));
}

int is_same_fspconfig(StreamProcessor *left, StreamProcessor *right)
{
  int config_same = 0 == memcmp(&left->config, &right->config, sizeof(FSPConfig));
  int buffer_same = left->buffer->max_states == right->buffer->max_states
  && left->buffer->buffer_window.seconds == right->buffer->buffer_window.seconds
  && left->buffer->buffer_window.nanoseconds == right->buffer->buffer_window.nanoseconds;

  int hwm_same = left->dsp_hwm->ntraces == right->dsp_hwm->ntraces
  && 0 == memcmp(left->dsp_hwm->tracemap, right->dsp_hwm->tracemap, FCIOMaxChannels)
  && 0 == memcmp(left->dsp_hwm->fpga_energy_threshold_adc, right->dsp_hwm->fpga_energy_threshold_adc, FCIOMaxChannels);

  int ct_same = left->dsp_ct->ntraces == right->dsp_ct->ntraces
  && 0 == memcmp(left->dsp_ct->tracemap, right->dsp_ct->tracemap, FCIOMaxChannels)
  && 0 == memcmp(left->dsp_ct->thresholds, right->dsp_ct->thresholds, FCIOMaxChannels);

  int wps_same = left->dsp_wps->ntraces == right->dsp_wps->ntraces
  && left->dsp_wps->apply_gain_scaling == right->dsp_wps->apply_gain_scaling
  && left->dsp_wps->coincidence_window == right->dsp_wps->coincidence_window
  && left->dsp_wps->sum_window_start_sample == right->dsp_wps->sum_window_start_sample
  && left->dsp_wps->sum_window_stop_sample == right->dsp_wps->sum_window_stop_sample
  && left->dsp_wps->coincidence_threshold == right->dsp_wps->coincidence_threshold
  && left->dsp_wps->dsp_pre_max_samples == right->dsp_wps->dsp_pre_max_samples
  && left->dsp_wps->dsp_post_max_samples == right->dsp_wps->dsp_post_max_samples
  && left->dsp_wps->apply_gain_scaling == right->dsp_wps->apply_gain_scaling
  && 0 == memcmp(left->dsp_wps->tracemap, right->dsp_wps->tracemap, FCIOMaxChannels)
  && 0 == memcmp(left->dsp_wps->gains, right->dsp_wps->gains, FCIOMaxChannels)
  && 0 == memcmp(left->dsp_wps->thresholds, right->dsp_wps->thresholds, FCIOMaxChannels)
  && 0 == memcmp(left->dsp_wps->lowpass, right->dsp_wps->lowpass, FCIOMaxChannels)
  && 0 == memcmp(left->dsp_wps->shaping_widths, right->dsp_wps->shaping_widths, FCIOMaxChannels)
  && 0 == memcmp(left->dsp_wps->dsp_pre_samples, right->dsp_wps->dsp_pre_samples, FCIOMaxChannels)
  && 0 == memcmp(left->dsp_wps->dsp_post_samples, right->dsp_wps->dsp_post_samples, FCIOMaxChannels)
  && 0 == memcmp(left->dsp_wps->dsp_start_sample, right->dsp_wps->dsp_start_sample, FCIOMaxChannels)
  && 0 == memcmp(left->dsp_wps->dsp_stop_sample, right->dsp_wps->dsp_stop_sample, FCIOMaxChannels);

  return config_same && buffer_same && hwm_same && ct_same && wps_same;
}

int is_same_fspevent(FSPState *left, FSPState *right)
{
  return 0 == memcmp(&left->write_flags, &right->write_flags, sizeof(FSPWriteFlags) )
  && 0 == memcmp(&left->proc_flags, &right->proc_flags, sizeof(FSPProcessorFlags) )
  && 0 == memcmp(&left->obs.evt, &right->obs.evt, sizeof(evt_obs) )
  && 0 == memcmp(&left->obs.hwm, &right->obs.hwm, sizeof(hwm_obs) )
  && 0 == memcmp(&left->obs.wps, &right->obs.wps, sizeof(wps_obs) )
  && left->obs.ct.multiplicity == right->obs.ct.multiplicity
  && 0 == memcmp(left->obs.ct.trace_idx, right->obs.ct.trace_idx, right->obs.ct.multiplicity )
  && 0 == memcmp(left->obs.ct.max, right->obs.ct.max, right->obs.ct.multiplicity )
  && left->obs.sub_event_list.size == right->obs.sub_event_list.size
  && 0 == memcmp(left->obs.sub_event_list.start, right->obs.sub_event_list.start, right->obs.sub_event_list.size )
  && 0 == memcmp(left->obs.sub_event_list.stop, right->obs.sub_event_list.stop, right->obs.sub_event_list.size )
  && 0 == memcmp(left->obs.sub_event_list.wps_max, right->obs.sub_event_list.wps_max, right->obs.sub_event_list.size )
  ;
}
