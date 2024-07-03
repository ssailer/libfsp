#pragma once

#include "fsp/timestamps.h"
#include "fsp/flags.h"
#include "fsp/stats.h"
#include "fsp/buffer.h"
#include "fsp/dsp.h"

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
  int minimum_buffer_depth;
  FSPBuffer *buffer;

  WindowedPeakSumConfig *wps_cfg;
  HardwareMajorityConfig *hwm_cfg;
  ChannelThresholdConfig *ct_cfg;
  FSPStats *stats;

} StreamProcessor;
