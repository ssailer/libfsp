#pragma once

#include <stdint.h>

#include <fcio.h>

#include <fsp_timestamps.h>
#include <fsp_dsp.h>

// #define ST_NSTATES 5
// typedef enum SoftwareTriggerFlags {

//   ST_NULL = 0,
//   ST_HWM_TRIGGER = 1 << 0,
//   ST_HWM_PRESCALED = 1 << 1,
//   ST_WPS_ABS_TRIGGER = 1 << 2,
//   ST_WPS_REL_TRIGGER = 1 << 3,
//   ST_WPS_PRESCALED = 1 << 4,
//   ST_CT_TRIGGER = 1 << 5,

// } SoftwareTriggerFlags;

// #define EVT_NSTATES 12
// typedef enum EventFlags {

//   EVT_NULL = 0,
//   EVT_RETRIGGER = 1 << 0,
//   EVT_EXTENDED = 1 << 1,
//   EVT_HWM_MULT_THRESHOLD = 1 << 2,
//   EVT_HWM_MULT_ENERGY_BELOW = 1 << 3,
//   EVT_WPS_ABS_THRESHOLD = 1 << 4,
//   EVT_WPS_REL_THRESHOLD = 1 << 5,
//   EVT_WPS_REL_REFERENCE = 1 << 6,
//   EVT_WPS_REL_PRE_WINDOW = 1 << 7,
//   EVT_WPS_REL_POST_WINDOW = 1 << 8,
//   EVT_CT_THRESHOLD = 1 << 9,
//   // EVT_DF_PULSER = 1 << 9,
//   // EVT_DF_BASELINE = 1 << 10,
//   // EVT_DF_MUON = 1 << 11,

// } EventFlags;

typedef union STFlags {
  struct {
    uint8_t hwm_multiplicity;
    uint8_t hwm_prescaled;
    uint8_t wps_abs;
    uint8_t wps_rel;
    uint8_t wps_prescaled;
    uint8_t ct_multiplicity;
  };
  uint64_t is_flagged;
} STFlags;

typedef union EventFlags {
  struct {
    uint8_t is_retrigger;
    uint8_t is_extended;
  };
  uint64_t is_flagged;
} EventFlags;

typedef union WPSFlags {
  struct {
    uint8_t abs_threshold;
    uint8_t rel_threshold;
    uint8_t rel_reference;
    uint8_t rel_pre_window;
    uint8_t rel_post_window;
  };
  uint64_t is_flagged;
} WPSFlags;

typedef union HWMFlags {
  struct {
    uint8_t multiplicity_threshold;
    uint8_t multiplicity_below;
  };
  uint64_t is_flagged;
} HWMFlags;

typedef union CTFlags {
  struct {
    uint8_t multiplicity;
    // uint8_t is_extended;
  };
  uint64_t is_flagged;
} CTFlags;

typedef struct FSPFlags {

  EventFlags event;
  STFlags trigger;

  WPSFlags wps;
  HWMFlags hwm;
  CTFlags ct;
  // unsigned int trigger;
  // unsigned int event;

} FSPFlags;

// Windows Peak Sum
typedef struct FSPObervables {
  struct wps_obs {
    float max_value; // what is the maximum PE within the integration windows
    int max_offset;  // when is the total sum offset reached?
    int max_multiplicity;  // How many channels did have a peak above thresholds
    float max_single_peak_value;   // which one was the largest individual peak
    int max_single_peak_offset;  // which sample contains this peak

    /* sub triggerlist */
    WPSTriggerList trigger_list;
  } wps;


  // FPGA Majority
  struct hwm_obs {
    int multiplicity;          // how many channels have fpga_energy > 0
    unsigned short max_value;  // what is the largest fpga_energy of those
    unsigned short min_value;  // what is the smallest fpga_energy of those

  } hwm;

  // Channel Threshold 
  struct ct_obs {
    int multiplicity;
    unsigned short max[FCIOMaxChannels];
    int trace_idx[FCIOMaxChannels];
    const char* label[FCIOMaxChannels];

  } ct;

  struct event_obs {
    int nextension;
  } evt;

} FSPObservables;

typedef struct FSPState {
  /* internal */
  FCIOState *state;
  Timestamp timestamp;
  Timestamp unixstamp;
  int has_timestamp;
  int in_buffer;
  int stream_tag;

  /* calculate observables if event */
  FSPObservables obs;
  /* condense observables into flags */
  FSPFlags flags;
  /* final write decision based on enabled flags */
  int write;

} FSPState;
