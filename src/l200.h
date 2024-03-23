#pragma once

#include <fsp.h>

int FSPSetAuxParameters(StreamProcessor *processor, const char *channelmap_format, int digital_pulser_channel,
                        int pulser_level_adc, int digital_baseline_channel, int baseline_level_adc,
                        int digital_muon_channel, int muon_level_adc);

int FSPSetGeParameters(StreamProcessor *processor, int nchannels, int *channelmap, const char *channelmap_format,
                       int majority_threshold, int skip_full_counting, unsigned short *ge_prescaling_threshold_adc,
                       float ge_average_prescaling_rate_hz);

int FSPSetSiPMParameters(StreamProcessor *processor, int nchannels, int *channelmap, const char *channelmap_format,
                         float *calibration_factors, float *channel_thresholds_pe, int *shaping_width_samples,
                         float *lowpass_factors, int coincidence_pre_window_ns, int coincidence_post_window_ns,
                         int coincidence_window_samples, int sum_window_start_sample, int sum_window_stop_sample,
                         float sum_threshold_pe, float coincidence_sum_threshold_pe, float average_prescaling_rate_hz,
                         int enable_muon_coincidence);
