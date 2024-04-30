#include "fsp_l200.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include <fsp_state.h>

// int FSPSetAuxParameters(StreamProcessor* processor, FSPChannelFormat format, int digital_pulser_channel,
//                         int pulser_level_adc, int digital_baseline_channel, int baseline_level_adc,
//                         int digital_muon_channel, int muon_level_adc) {
//   if (!is_known_channelmap_format(format)) {
//     if (processor->loglevel)
//       fprintf(stderr,
//               "ERROR LPPSetAuxParameters: channel map type %s is not supported. Valid inputs are \"fcio-trace-index\", "
//               "\"fcio-tracemap\" or \"rawid\".\n",
//               channelmap_fmt2str(format));
//     return 0;
//   }
//   processor->aux.tracemap_format = format;

//   processor->aux.pulser_trace_index = digital_pulser_channel;
//   processor->aux.pulser_adc_threshold = pulser_level_adc;
//   processor->aux.baseline_trace_index = digital_baseline_channel;
//   processor->aux.baseline_adc_threshold = baseline_level_adc;
//   processor->aux.muon_trace_index = digital_muon_channel;
//   processor->aux.muon_adc_threshold = muon_level_adc;

//   if (processor->loglevel >= 4) {
//     fprintf(stderr, "DEBUG LPPSetAuxParameters\n");
//     fprintf(stderr, "DEBUG channelmap_format %d : %s\n", processor->aux.tracemap_format, channelmap_fmt2str(format));
//     if (processor->aux.tracemap_format == 1) {
//       fprintf(stderr, "DEBUG digital_pulser_channel   0x%x level_adc %d\n", processor->aux.pulser_trace_index,
//               processor->aux.pulser_adc_threshold);
//       fprintf(stderr, "DEBUG digital_baseline_channel 0x%x level_adc %d\n", processor->aux.baseline_trace_index,
//               processor->aux.baseline_adc_threshold);
//       fprintf(stderr, "DEBUG digital_muon_channel     0x%x level_adc %d\n", processor->aux.muon_trace_index,
//               processor->aux.muon_adc_threshold);
//     } else {
//       fprintf(stderr, "DEBUG digital_pulser_channel   %d level_adc %d\n", processor->aux.pulser_trace_index,
//               processor->aux.pulser_adc_threshold);
//       fprintf(stderr, "DEBUG digital_baseline_channel %d level_adc %d\n", processor->aux.baseline_trace_index,
//               processor->aux.baseline_adc_threshold);
//       fprintf(stderr, "DEBUG digital_muon_channel     %d level_adc %d\n", processor->aux.muon_trace_index,
//               processor->aux.muon_adc_threshold);
//     }
//   }
//   return 1;
// }

int FSPSetAuxParameters(StreamProcessor* processor, FSPChannelFormat format, int pulser_channel,
                        int pulser_level_adc, int baseline_channel, int baseline_level_adc,
                        int muon_channel, int muon_level_adc) {
  if (!is_known_channelmap_format(format)) {
    if (processor->loglevel)
      fprintf(stderr,
              "ERROR LPPSetAuxParameters: channel map type %s is not supported. Valid inputs are \"fcio-trace-index\", "
              "\"fcio-tracemap\" or \"rawid\".\n",
              channelmap_fmt2str(format));
    return 0;
  }
  processor->ct_cfg = calloc(1, sizeof(ChannelThresholdConfig));

  ChannelThresholdConfig* ct_cfg = processor->ct_cfg;

  ct_cfg->tracemap_format = format;
  ct_cfg->tracemap[0] = pulser_channel;
  ct_cfg->tracemap[1] = baseline_channel;
  ct_cfg->tracemap[2] = muon_channel;
  ct_cfg->thresholds[0] = pulser_level_adc;
  ct_cfg->thresholds[1] = baseline_level_adc;
  ct_cfg->thresholds[2] = muon_level_adc;
  ct_cfg->labels[0] = "Pulser";
  ct_cfg->labels[1] = "Baseline";
  ct_cfg->labels[2] = "Muon";
  ct_cfg->ntraces = 3;

  if (processor->loglevel >= 4) {
    fprintf(stderr, "DEBUG LPPSetAuxParameters\n");
    for (int i = 0; i < ct_cfg->ntraces; i++) {
      if (ct_cfg->tracemap_format == FCIO_TRACE_MAP_FORMAT) {
        fprintf(stderr, "DEBUG %s channel   0x%x level_adc %d\n", ct_cfg->labels[i], ct_cfg->tracemap[i],
                ct_cfg->thresholds[i]);
      } else {
        fprintf(stderr, "DEBUG %s channel   %d level_adc %d\n", ct_cfg->labels[i], ct_cfg->tracemap[i],
                ct_cfg->thresholds[i]);
      }
    }
  }
  return 1;
}

int FSPSetGeParameters(StreamProcessor* processor, int nchannels, int* channelmap, FSPChannelFormat format,
                       int majority_threshold, int skip_full_counting, unsigned short* ge_prescaling_threshold_adc,
                       float ge_average_prescaling_rate_hz) {
  processor->hwm_cfg = calloc(1, sizeof(HardwareMajorityConfig));
  HardwareMajorityConfig* fmc = processor->hwm_cfg;

  if (!is_known_channelmap_format(format)) {
    if (processor->loglevel)
      fprintf(stderr,
              "ERROR LPPSetGeParameters: channel map type %s is not supported. Valid inputs are \"fcio-trace-index\", "
              "\"fcio-tracemap\" or \"rawid\".\n",
              channelmap_fmt2str(format));
    free(fmc);
    return 0;
  }
  fmc->tracemap_format = format;
  fmc->ntraces = nchannels;

  for (int i = 0; i < nchannels && i < FCIOMaxChannels; i++) {
    fmc->tracemap[i] = channelmap[i];
    fmc->fpga_energy_threshold_adc[i] = ge_prescaling_threshold_adc[i];
  }
  fmc->fast = skip_full_counting;
  if (majority_threshold >= 0)
    processor->hwm_threshold = majority_threshold;
  else {
    fprintf(stderr, "CRITICAL majority_threshold needs to be >= 0 is %d\n", majority_threshold);
    return 0;
  }
  if (ge_average_prescaling_rate_hz >= 0.0)
    processor->hwm_prescaling_rate = ge_average_prescaling_rate_hz;
  else {
    fprintf(stderr, "CRITICAL ge_average_prescaling_rate_hz needs to be >= 0.0 is %f\n", ge_average_prescaling_rate_hz);
    return 0;
  }

  if (processor->loglevel >= 4) {
    fprintf(stderr, "DEBUG LPPSetGeParameters\n");
    fprintf(stderr, "DEBUG majority_threshold %d\n", majority_threshold);
    fprintf(stderr, "DEBUG average_prescaling_rate_hz %f\n", ge_average_prescaling_rate_hz);
    fprintf(stderr, "DEBUG skip_full_counting %d\n", fmc->fast);
    fprintf(stderr, "DEBUG channelmap_format %d : %s\n", fmc->tracemap_format, channelmap_fmt2str(format));
    for (int i = 0; i < fmc->ntraces; i++) {
      if (fmc->tracemap_format == FCIO_TRACE_MAP_FORMAT) {
        fprintf(stderr, "DEBUG channel 0x%x\n", fmc->tracemap[i]);
      } else {
        fprintf(stderr, "DEBUG channel %d\n", fmc->tracemap[i]);
      }
    }
  }
  return 1;
}

int FSPSetSiPMParameters(StreamProcessor* processor, int nchannels, int* channelmap, FSPChannelFormat format,
                         float* calibration_pe_adc, float* channel_thresholds_pe, int* shaping_width_samples,
                         float* lowpass_factors, int coincidence_pre_window_ns, int coincidence_post_window_ns,
                         int coincidence_window_samples, int sum_window_start_sample, int sum_window_stop_sample,
                         float sum_threshold_pe, float coincidence_wps_threshold, float average_prescaling_rate_hz,
                         int enable_muon_coincidence) {
  processor->wps_cfg = calloc(1, sizeof(WindowedPeakSumConfig));
  WindowedPeakSumConfig* wps_cfg = processor->wps_cfg;

  if (!is_known_channelmap_format(format)) {
    if (processor->loglevel)
      fprintf(stderr,
              "CRITICAL LPPSetSiPMParameters: channel map type %s is not supported. Valid inputs are "
              "\"fcio-trace-index\", \"fcio-tracemap\" or \"rawid\".\n",
              channelmap_fmt2str(format));
    free(wps_cfg);
    return 0;
  }
  wps_cfg->tracemap_format = format;

  if (coincidence_wps_threshold >= 0)
    processor->relative_wps_threshold = coincidence_wps_threshold;
  else {
    fprintf(stderr, "CRICITAL coincidence_wps_threshold needs to be >= 0 is %f\n", coincidence_wps_threshold);
    return 0;
  }

  if (sum_threshold_pe >= 0)
    processor->absolute_wps_threshold = sum_threshold_pe;
  else {
    fprintf(stderr, "CRICITAL sum_threshold_pe needs to be >= 0 is %f\n", sum_threshold_pe);
    return 0;
  }

  processor->pre_trigger_window.seconds = coincidence_pre_window_ns / 1000000000L;
  processor->pre_trigger_window.nanoseconds = coincidence_pre_window_ns % 1000000000L;
  processor->post_trigger_window.seconds = coincidence_post_window_ns / 1000000000L;
  processor->post_trigger_window.nanoseconds = coincidence_post_window_ns % 1000000000L;
  processor->wps_prescaling_rate = average_prescaling_rate_hz;
  if (processor->wps_prescaling_rate > 0.0)
    processor->wps_prescaling = "average";  // could be "offset" when selecting ->wps_prescaling_offset, but is disabled here.
  else
    processor->wps_prescaling = NULL;
  processor->muon_coincidence = enable_muon_coincidence;

  // wps_cfg->repetition = processor->fast?4:sma_repetition;
  wps_cfg->coincidence_window = coincidence_window_samples;
  wps_cfg->sum_window_start_sample = sum_window_start_sample;
  wps_cfg->sum_window_stop_sample = sum_window_stop_sample;
  wps_cfg->coincidence_threshold = coincidence_wps_threshold;
  // wps_cfg->trigger_list.size = 0;
  // wps_cfg->trigger_list.threshold = coincidence_wps_threshold; // use the same threshold as the processor to check for the flag

  /* TODO CHECK THIS*/
  wps_cfg->apply_gain_scaling = 1;

  wps_cfg->ntraces = nchannels;

  wps_cfg->dsp_pre_max_samples = 0;
  wps_cfg->dsp_post_max_samples = 0;
  for (int i = 0; i < nchannels && i < FCIOMaxChannels; i++) {
    wps_cfg->tracemap[i] = channelmap[i];

    if (calibration_pe_adc[i] >= 0) {
      wps_cfg->gains[i] = calibration_pe_adc[i];
    } else {
      fprintf(stderr, "CRITICAL calibration_pe_adc for channel[%d] = %d needs to be >= 0 is %f\n", i, channelmap[i], calibration_pe_adc[i]);
      return 0;
    }

    if (channel_thresholds_pe[i] >= 0) {
      wps_cfg->thresholds[i] = channel_thresholds_pe[i];
    } else {
      fprintf(stderr, "CRITICAL channel_thresholds_pe for channel[%d] = %d needs to be >= 0 is %f\n", i, channelmap[i], channel_thresholds_pe[i]);
      return 0;
    }

    if (shaping_width_samples[i] >= 1) {
      wps_cfg->shaping_widths[i] = shaping_width_samples[i];
    } else {
      fprintf(stderr, "CRITICAL shaping_width_samples for channel[%d] = %d needs to be >= 1 is %d\n", i, channelmap[i], shaping_width_samples[i]);
      return 0;
    }

    if (lowpass_factors[i] >= 0) {
      wps_cfg->lowpass[i] = lowpass_factors[i];
    } else {
      fprintf(stderr, "CRITICAL lowpass_factors for channel[%d] = %d needs to be >= 0 is %f\n", i, channelmap[i], lowpass_factors[i]);
      return 0;
    }

    wps_cfg->dsp_pre_samples[i] = fsp_dsp_diff_and_smooth_pre_samples(shaping_width_samples[i], wps_cfg->lowpass[i]);
    if (wps_cfg->dsp_pre_samples[i] > wps_cfg->dsp_pre_max_samples) wps_cfg->dsp_pre_max_samples = wps_cfg->dsp_pre_samples[i];
    wps_cfg->dsp_post_samples[i] = fsp_dsp_diff_and_smooth_post_samples(shaping_width_samples[i], wps_cfg->lowpass[i]);
    if (wps_cfg->dsp_post_samples[i] > wps_cfg->dsp_post_max_samples) wps_cfg->dsp_post_max_samples = wps_cfg->dsp_post_samples[i];
  }

  if (processor->loglevel >= 4) {
    /* DEBUGGING enabled, print all inputs */
    fprintf(stderr, "DEBUG LPPSetSiPMParameters:\n");
    fprintf(stderr, "DEBUG channelmap_format %d : %s\n", wps_cfg->tracemap_format, channelmap_fmt2str(format));
    fprintf(stderr, "DEBUG average_prescaling_rate_hz   %f\n", processor->wps_prescaling_rate);
    fprintf(stderr, "DEBUG sum_window_start_sample      %d\n", wps_cfg->sum_window_start_sample);
    fprintf(stderr, "DEBUG sum_window_stop_sample       %d\n", wps_cfg->sum_window_stop_sample);
    fprintf(stderr, "DEBUG dsp_pre_max_samples          %d\n", wps_cfg->dsp_pre_max_samples);
    fprintf(stderr, "DEBUG dsp_post_max_samples         %d\n", wps_cfg->dsp_post_max_samples);
    fprintf(stderr, "DEBUG coincidence_pre_window_ns    %ld\n", processor->pre_trigger_window.nanoseconds);
    fprintf(stderr, "DEBUG coincidence_post_window_ns   %ld\n", processor->post_trigger_window.nanoseconds);
    fprintf(stderr, "DEBUG coincidence_window_samples   %d\n", wps_cfg->coincidence_window);
    fprintf(stderr, "DEBUG relative_wps_threshold       %f\n", processor->relative_wps_threshold);
    fprintf(stderr, "DEBUG absolute_sum_threshold       %f\n", processor->absolute_wps_threshold);
    fprintf(stderr, "DEBUG enable_muon_coincidence      %d\n", processor->muon_coincidence);

    for (int i = 0; i < wps_cfg->ntraces; i++) {
      if (wps_cfg->tracemap_format == 1) {
        fprintf(
            stderr,
            "DEBUG channel 0x%x gain %f threshold %f shaping %d lowpass %f dsp_pre_samples %d dsp_post_samples %d\n",
            wps_cfg->tracemap[i], wps_cfg->gains[i], wps_cfg->thresholds[i], wps_cfg->shaping_widths[i], wps_cfg->lowpass[i],
            wps_cfg->dsp_pre_samples[i], wps_cfg->dsp_post_samples[i]);
      } else {
        fprintf(stderr,
                "DEBUG channel %d gain %f threshold %f shaping %d lowpass %f dsp_pre_samples %d dsp_post_samples %d\n",
                wps_cfg->tracemap[i], wps_cfg->gains[i], wps_cfg->thresholds[i], wps_cfg->shaping_widths[i], wps_cfg->lowpass[i],
                wps_cfg->dsp_pre_samples[i], wps_cfg->dsp_post_samples[i]);
      }
    }
  }
  return 1;
}

static inline size_t event_flag_2char(char* string, size_t strlen, EventFlags event_flags) {
  const int nflags = 2;
  assert(strlen >= nflags);

  int written = 0;
  
  // if (event_flags & EVT_RETRIGGER) string[written] = 'R';
  // if (event_flags & EVT_EXTENDED) string[written] = 'E';
  // if (event_flags & EVT_CT_THRESHOLD) string[written] = 'T';
  // if (event_flags & EVT_HWM_MULT_THRESHOLD) string[written] = 'M';
  // if (event_flags & EVT_HWM_MULT_ENERGY_BELOW) string[written] = 'L';
  // if (event_flags & EVT_WPS_REL_REFERENCE) string[written] = '!';
  // if (event_flags & EVT_WPS_REL_POST_WINDOW) string[written] = '<';
  // if (event_flags & EVT_WPS_REL_THRESHOLD) string[written] = '-';
  // if (event_flags & EVT_WPS_REL_PRE_WINDOW) string[written] = '>';
  // string[10] = '\0';
  string[written++] = ':';
  if (event_flags.is_retrigger)
    string[written] = 'R';
  if (event_flags.is_extended)
    string[written] = 'E';
  
  written++;
  return written;
}

static inline size_t ct_flag_2char(char* string, size_t strlen, CTFlags ct_flags) {
  const int nflags = 2;
  assert(strlen >= nflags);

  int written = 0;
  string[written++] = ':';
  if (ct_flags.multiplicity) string[written] = 'T';
  written++;
  return written;
}

static inline size_t hwm_flag_2char(char* string, size_t strlen, HWMFlags hwm_flags) {
  const int nflags = 3;
  assert(strlen >= nflags);

  int written = 0;
  string[written++] = ':';
  if (hwm_flags.multiplicity_threshold) string[written] = 'M';
  written++;
  if (hwm_flags.multiplicity_below) string[written] = 'L';
  written++;
  return written;
}

static inline size_t wps_flag_2char(char* string, size_t strlen, WPSFlags wps_flags) {
  const int nflags = 6;
  assert(strlen >= nflags);

  int written = 0;
  string[written++] = ':';
  if (wps_flags.rel_reference) string[written] = '!' ;
  written++;
  if (wps_flags.rel_post_window) string[written] = '<' ;
  written++;
  if (wps_flags.rel_threshold) string[written] = '-' ;
  written++;
  if (wps_flags.rel_pre_window) string[written] = '>' ;
  written++;
  if (wps_flags.abs_threshold) string[written] = 'A';
  written++;
  return written;
}

static inline size_t st_flag_2char(char* string, size_t strlen, STFlags st_flags) {
  const int nflags = 7;
  assert(strlen >= nflags);

  int written = 0;
  string[written++] = ':';

  if (st_flags.hwm_multiplicity) string[written] = 'M' ;
  written++;
  if (st_flags.hwm_prescaled) string[written] = 'G' ;
  written++;
  if (st_flags.wps_abs) string[written] = 'A' ;
  written++;
  if (st_flags.wps_rel) string[written] = 'C' ;
  written++;
  if (st_flags.wps_prescaled) string[written] = 'S' ;
  written++;
  if (st_flags.ct_multiplicity) string[written] = 'T' ;
  written++;

  return written;
}

void FSPFlags2Char(FSPState* fsp_state, size_t strlen, char* cstring) {
  const int nfields = 9 + 6 + 1 + 1 + 2 + 5;
  assert(strlen >= nfields);

  for (size_t i = 0; i < nfields; i++) cstring[i] = '_';
  size_t curr_offset = 0;
  cstring[curr_offset++] = fsp_state->write ? 'W' : 'D';

  switch (fsp_state->stream_tag) {
    case FCIOConfig:
      cstring[curr_offset++] = 'C';
      break;
    case FCIOStatus:
      cstring[curr_offset++] = 'S';
      break;
    case FCIOEvent:
      cstring[curr_offset++] = 'E';
      break;
    case FCIOSparseEvent:
      cstring[curr_offset++] = 'Z';
      break;
    case FCIORecEvent:
      cstring[curr_offset++] = 'R';
      break;
    default:
      cstring[curr_offset++] = '?';
      break;
  }

  curr_offset += st_flag_2char(&cstring[curr_offset], 7, fsp_state->flags.trigger);

  curr_offset += event_flag_2char(&cstring[curr_offset], 2, fsp_state->flags.event);

  curr_offset += ct_flag_2char(&cstring[curr_offset], 2, fsp_state->flags.ct);

  curr_offset += hwm_flag_2char(&cstring[curr_offset], 3, fsp_state->flags.hwm);

  curr_offset += wps_flag_2char(&cstring[curr_offset], 6, fsp_state->flags.wps);

  cstring[curr_offset++] = ':';
  for (int i = 0; curr_offset < strlen && i < fsp_state->obs.ct.multiplicity; i++, curr_offset++) {
    cstring[curr_offset] = fsp_state->obs.ct.label[i][0];
  }
  cstring[curr_offset] = '\0';
}

void FSPFlags2BitField(FSPFlags flags, uint32_t* trigger_field, uint32_t* event_field)
{
  uint32_t tfield = 0;
  uint32_t efield = 0;

  tfield |= ((flags.trigger.hwm_multiplicity & 0x1) << 0);
  tfield |= ((flags.trigger.hwm_prescaled & 0x1)    << 1);
  tfield |= ((flags.trigger.wps_abs & 0x1)          << 2);
  tfield |= ((flags.trigger.wps_rel & 0x1)          << 3);
  tfield |= ((flags.trigger.wps_prescaled & 0x1)    << 4);
  tfield |= ((flags.trigger.ct_multiplicity & 0x1)  << 5);

  efield |= ((flags.event.is_extended & 0x1)          << 0);
  efield |= ((flags.event.is_retrigger & 0x1)         << 1);
  efield |= ((flags.wps.abs_threshold & 0x1)          << 2);
  efield |= ((flags.wps.rel_threshold & 0x1)          << 3);
  efield |= ((flags.wps.rel_reference & 0x1)          << 4);
  efield |= ((flags.wps.rel_pre_window & 0x1)         << 5);
  efield |= ((flags.wps.rel_post_window & 0x1)        << 6);
  efield |= ((flags.hwm.multiplicity_threshold & 0x1) << 7);
  efield |= ((flags.hwm.multiplicity_below & 0x1)     << 8);
  efield |= ((flags.ct.multiplicity & 0x1)            << 9);
  
  *trigger_field = tfield;
  *event_field = efield;
}

void FSPFlags2BitString(FSPFlags flags, size_t strlen, char* trigger_string, char* event_string)
{
  assert(strlen >= 13);

  char* trgstring = &trigger_string[8];
  char* evtstring = &event_string[12];
  
  *trgstring-- = 0;
  *trgstring-- = (flags.trigger.hwm_multiplicity & 0x1) ? '1' : '0';
  *trgstring-- = (flags.trigger.hwm_prescaled & 0x1) ? '1' : '0';
  *trgstring-- = (flags.trigger.wps_abs & 0x1) ? '1' : '0';
  *trgstring-- = (flags.trigger.wps_rel & 0x1) ? '1' : '0';
  *trgstring-- = (flags.trigger.wps_prescaled & 0x1) ? '1' : '0';
  *trgstring-- = (flags.trigger.ct_multiplicity & 0x1) ? '1' : '0';
  *trgstring-- = 'b';
  *trgstring = '0';

  *evtstring-- = 0;
  *evtstring-- = (flags.event.is_extended & 0x1) ? '1' : '0';
  *evtstring-- = (flags.event.is_retrigger & 0x1) ? '1' : '0';
  *evtstring-- = (flags.wps.abs_threshold & 0x1) ? '1' : '0';
  *evtstring-- = (flags.wps.rel_threshold & 0x1) ? '1' : '0';
  *evtstring-- = (flags.wps.rel_reference & 0x1) ? '1' : '0';
  *evtstring-- = (flags.wps.rel_pre_window & 0x1) ? '1' : '0';
  *evtstring-- = (flags.wps.rel_post_window & 0x1) ? '1' : '0';
  *evtstring-- = (flags.hwm.multiplicity_threshold & 0x1) ? '1' : '0';
  *evtstring-- = (flags.hwm.multiplicity_below & 0x1) ? '1' : '0';
  *evtstring-- = (flags.ct.multiplicity & 0x1) ? '1' : '0';
  *evtstring-- = 'b';
  *evtstring-- = '0';
}
