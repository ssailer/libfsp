#pragma once

#include "fsp/processor.h"
#include "fsp/tracemap.h"

static inline int load_sipm_tracemap(int max_channels, int *channels) {
  const int last_card = 10;
  const int max_card_slots = 6;  // 16-bit firmware
  const int baseaddress = 0x200;

  int nchannels = 0;
  for (int i = 1; i <= last_card; i++) {
    int card_address = baseaddress + i * 0x10;  // 0x10 == 16
    for (int j = 0; j < max_card_slots && nchannels < max_channels; j++) {
      if (i == 1 && j < 2) {
        // First 2 Channels not connected
        continue;
      }
      channels[nchannels++] = (card_address << 16) + j;
    }
  }
  return nchannels;
}

static inline int load_hpge_tracemap(int max_channels, int *channels) {
  const int last_card = 10;
  const int max_card_slots = 6;  // 16-bit firmware
  const int baseaddresses[] = {0x300, 0x400};
  const int max_crates = sizeof(baseaddresses)/sizeof(*baseaddresses);

  int nchannels = 0;
  for (int k = 0; k < max_crates; k++) {
    const int baseaddress = baseaddresses[k];
    for (int i = 1; i <= last_card; i++) {
      int card_address = baseaddress + i * 0x10;  // 0x10 == 16
      for (int j = 0; j < max_card_slots && nchannels < max_channels; j++) {
        channels[nchannels++] = (card_address << 16) + j;
      }
    }
  }

  return nchannels;
}

int FSP_L200_SetAuxParameters(StreamProcessor *processor, FSPTraceFormat format, int digital_pulser_channel,
                        int pulser_level_adc, int digital_baseline_channel, int baseline_level_adc,
                        int digital_muon_channel, int muon_level_adc);

int FSP_L200_SetGeParameters(StreamProcessor *processor, int nchannels, int *channelmap, FSPTraceFormat format,
                       int majority_threshold, int skip_full_counting, unsigned short *ge_prescaling_threshold_adc,
                       int prescale_ratio);

int FSP_L200_SetSiPMParameters(StreamProcessor *processor, int nchannels, int *channelmap, FSPTraceFormat format,
                         float *calibration_factors, float *channel_thresholds_pe, int *shaping_width_samples,
                         float *lowpass_factors, int coincidence_pre_window_ns, int coincidence_post_window_ns,
                         int coincidence_window_samples, int sum_window_start_sample, int sum_window_stop_sample,
                         float sum_threshold_pe, float coincidence_sum_threshold_pe, int prescale_ratio,
                         int enable_muon_coincidence);
