#include "io.h"
#include "fsp/buffer.h"
#include "fsp/dsp.h"
#include "fsp/processor.h"

#include <assert.h>
#include <stdlib.h>

#include <fcio.h>
#include <bufio.h>
#include <tmio.h>

static inline size_t event_flag_2char(char* string, size_t strlen, EventFlags event_flags) {
  const int nflags = 2;
  assert(strlen >= nflags);

  int written = 0;

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
  cstring[curr_offset++] = fsp_state->write_flags.write ? 'W' : 'D';

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

  curr_offset += st_flag_2char(&cstring[curr_offset], 7, fsp_state->write_flags.trigger);

  curr_offset += event_flag_2char(&cstring[curr_offset], 2, fsp_state->write_flags.event);

  curr_offset += ct_flag_2char(&cstring[curr_offset], 2, fsp_state->proc_flags.ct);

  curr_offset += hwm_flag_2char(&cstring[curr_offset], 3, fsp_state->proc_flags.hwm);

  curr_offset += wps_flag_2char(&cstring[curr_offset], 6, fsp_state->proc_flags.wps);

  cstring[curr_offset++] = ':';
  for (int i = 0; curr_offset < strlen && i < fsp_state->obs.ct.multiplicity; i++, curr_offset++) {
    cstring[curr_offset] = (char)i;
  }
  cstring[curr_offset] = '\0';
}

void FSPFlags2BitField(FSPState* fsp_state, uint32_t* trigger_field, uint32_t* event_field)
{
  uint32_t tfield = 0;
  uint32_t efield = 0;

  tfield |= ((fsp_state->write_flags.trigger.hwm_multiplicity & 0x1) << 0);
  tfield |= ((fsp_state->write_flags.trigger.hwm_prescaled & 0x1)    << 1);
  tfield |= ((fsp_state->write_flags.trigger.wps_abs & 0x1)          << 2);
  tfield |= ((fsp_state->write_flags.trigger.wps_rel & 0x1)          << 3);
  tfield |= ((fsp_state->write_flags.trigger.wps_prescaled & 0x1)    << 4);
  tfield |= ((fsp_state->write_flags.trigger.ct_multiplicity & 0x1)  << 5);

  efield |= ((fsp_state->write_flags.event.is_extended & 0x1)          << 0);
  efield |= ((fsp_state->write_flags.event.is_retrigger & 0x1)         << 1);
  efield |= ((fsp_state->proc_flags.wps.abs_threshold & 0x1)          << 2);
  efield |= ((fsp_state->proc_flags.wps.rel_threshold & 0x1)          << 3);
  efield |= ((fsp_state->proc_flags.wps.rel_reference & 0x1)          << 4);
  efield |= ((fsp_state->proc_flags.wps.rel_pre_window & 0x1)         << 5);
  efield |= ((fsp_state->proc_flags.wps.rel_post_window & 0x1)        << 6);
  efield |= ((fsp_state->proc_flags.hwm.multiplicity_threshold & 0x1) << 7);
  efield |= ((fsp_state->proc_flags.hwm.multiplicity_below & 0x1)     << 8);
  efield |= ((fsp_state->proc_flags.ct.multiplicity & 0x1)            << 9);

  *trigger_field = tfield;
  *event_field = efield;
}

void FSPBitField2Flags(FSPState* fsp_state, uint32_t trigger_field, uint32_t event_field)
{
  fsp_state->write_flags.trigger.hwm_multiplicity =  trigger_field & (0x1 << 0);
  fsp_state->write_flags.trigger.hwm_prescaled =     trigger_field & (0x1 << 1);
  fsp_state->write_flags.trigger.wps_abs =           trigger_field & (0x1 << 2);
  fsp_state->write_flags.trigger.wps_rel =           trigger_field & (0x1 << 3);
  fsp_state->write_flags.trigger.wps_prescaled =     trigger_field & (0x1 << 4);
  fsp_state->write_flags.trigger.ct_multiplicity =   trigger_field & (0x1 << 5);

  fsp_state->write_flags.event.is_extended =           event_field & (0x1 << 0);
  fsp_state->write_flags.event.is_retrigger =          event_field & (0x1 << 1);
  fsp_state->proc_flags.wps.abs_threshold =           event_field & (0x1 << 2);
  fsp_state->proc_flags.wps.rel_threshold =           event_field & (0x1 << 3);
  fsp_state->proc_flags.wps.rel_reference =           event_field & (0x1 << 4);
  fsp_state->proc_flags.wps.rel_pre_window =          event_field & (0x1 << 5);
  fsp_state->proc_flags.wps.rel_post_window =         event_field & (0x1 << 6);
  fsp_state->proc_flags.hwm.multiplicity_threshold =  event_field & (0x1 << 7);
  fsp_state->proc_flags.hwm.multiplicity_below =      event_field & (0x1 << 8);
  fsp_state->proc_flags.ct.multiplicity =             event_field & (0x1 << 9);
}


void FSPFlags2BitString(FSPState* fsp_state, size_t strlen, char* trigger_string, char* event_string)
{
  assert(strlen >= 20);

  char* trgstring = &trigger_string[8];
  char* evtstring = &event_string[12];

  *trgstring-- = 0;
  *trgstring-- = (fsp_state->write_flags.trigger.hwm_multiplicity & 0x1) ? '1' : '0';
  *trgstring-- = (fsp_state->write_flags.trigger.hwm_prescaled & 0x1) ? '1' : '0';
  *trgstring-- = (fsp_state->write_flags.trigger.wps_abs & 0x1) ? '1' : '0';
  *trgstring-- = (fsp_state->write_flags.trigger.wps_rel & 0x1) ? '1' : '0';
  *trgstring-- = (fsp_state->write_flags.trigger.wps_prescaled & 0x1) ? '1' : '0';
  *trgstring-- = (fsp_state->write_flags.trigger.ct_multiplicity & 0x1) ? '1' : '0';
  *trgstring-- = 'b';
  *trgstring = '0';

  *evtstring-- = 0;
  *evtstring-- = (fsp_state->write_flags.event.is_extended & 0x1) ? '1' : '0';
  *evtstring-- = (fsp_state->write_flags.event.is_retrigger & 0x1) ? '1' : '0';
  *evtstring-- = (fsp_state->proc_flags.wps.abs_threshold & 0x1) ? '1' : '0';
  *evtstring-- = (fsp_state->proc_flags.wps.rel_threshold & 0x1) ? '1' : '0';
  *evtstring-- = (fsp_state->proc_flags.wps.rel_reference & 0x1) ? '1' : '0';
  *evtstring-- = (fsp_state->proc_flags.wps.rel_pre_window & 0x1) ? '1' : '0';
  *evtstring-- = (fsp_state->proc_flags.wps.rel_post_window & 0x1) ? '1' : '0';
  *evtstring-- = (fsp_state->proc_flags.hwm.multiplicity_threshold & 0x1) ? '1' : '0';
  *evtstring-- = (fsp_state->proc_flags.hwm.multiplicity_below & 0x1) ? '1' : '0';
  *evtstring-- = (fsp_state->proc_flags.ct.multiplicity & 0x1) ? '1' : '0';
  *evtstring-- = 'b';
  *evtstring-- = '0';
}

static inline int fcio_put_fspconfig_buffer(FCIOStream stream, FSPBuffer* buffer) {
  if (!stream || !buffer)
    return -1;
  FCIOWriteInt(stream, buffer->max_states);
  FCIOWrite(stream, sizeof(buffer->buffer_window.seconds), &buffer->buffer_window.seconds);
  FCIOWrite(stream, sizeof(buffer->buffer_window.nanoseconds), &buffer->buffer_window.nanoseconds);
  return 0;
}

static inline int fcio_put_fspconfig_hwm(FCIOStream stream, DSPHardwareMajority* hwm_cfg) {
  if (!stream || !hwm_cfg)
    return -1;
  FCIOWriteInts(stream, hwm_cfg->ntraces, hwm_cfg->tracemap);
  FCIOWriteUShorts(stream, hwm_cfg->ntraces, hwm_cfg->fpga_energy_threshold_adc);

  return 0;
}

static inline int fcio_put_fspconfig_ct(FCIOStream stream, DSPChannelThreshold* ct_cfg) {
  if (!stream || !ct_cfg)
    return -1;
  FCIOWriteInts(stream, ct_cfg->ntraces, ct_cfg->tracemap);
  FCIOWriteUShorts(stream, ct_cfg->ntraces, ct_cfg->thresholds);

  return 0;
}

static inline int fcio_put_fspconfig_wps(FCIOStream stream, DSPWindowedPeakSum* wps_cfg) {
  if (!stream || !wps_cfg)
    return -1;
  FCIOWriteInt(stream, wps_cfg->apply_gain_scaling);
  FCIOWriteInt(stream, wps_cfg->coincidence_window);
  FCIOWriteInt(stream, wps_cfg->sum_window_start_sample);
  FCIOWriteInt(stream, wps_cfg->sum_window_stop_sample);
  FCIOWriteFloat(stream, wps_cfg->coincidence_threshold);

  FCIOWriteInts(stream, wps_cfg->ntraces, wps_cfg->tracemap);
  FCIOWriteFloats(stream, wps_cfg->ntraces, wps_cfg->gains);
  FCIOWriteFloats(stream, wps_cfg->ntraces, wps_cfg->thresholds);
  FCIOWriteFloats(stream, wps_cfg->ntraces, wps_cfg->lowpass);
  FCIOWriteFloats(stream, wps_cfg->ntraces, wps_cfg->shaping_widths);

  FCIOWriteInt(stream, wps_cfg->dsp_pre_max_samples);
  FCIOWriteInt(stream, wps_cfg->dsp_post_max_samples);
  FCIOWriteInts(stream, wps_cfg->ntraces, wps_cfg->dsp_pre_samples);
  FCIOWriteInts(stream, wps_cfg->ntraces, wps_cfg->dsp_post_samples);
  FCIOWriteInts(stream, wps_cfg->ntraces, wps_cfg->dsp_start_sample);
  FCIOWriteInts(stream, wps_cfg->ntraces, wps_cfg->dsp_stop_sample);

  return 0;
}

int FCIOPutFSPConfig(FCIOStream output, StreamProcessor* processor)
{
  if (!output || !processor)
    return -1;

  FCIOWriteMessage(output, FCIOFSPConfig);

  /* StreamProcessor config */
  FCIOWrite(output, sizeof(processor->config), &processor->config);

  fcio_put_fspconfig_buffer(output, processor->buffer);
  fcio_put_fspconfig_hwm(output, processor->dsp_hwm);
  fcio_put_fspconfig_ct(output, processor->dsp_ct);
  fcio_put_fspconfig_wps(output, processor->dsp_wps);

  return FCIOFlush(output);
}


static inline int fcio_get_fspconfig_buffer(FCIOStream in, FSPBuffer* buffer) {
  if (!buffer)
    return -1;
  FCIOReadInt(in, buffer->max_states);
  FCIORead(in, sizeof(buffer->buffer_window.seconds), &buffer->buffer_window.seconds);
  FCIORead(in, sizeof(buffer->buffer_window.nanoseconds), &buffer->buffer_window.nanoseconds);
  return 0;
}

static inline int fcio_get_fspconfig_hwm(FCIOStream in, DSPHardwareMajority* hwm_cfg) {
  if (!hwm_cfg)
    return -1;
  hwm_cfg->ntraces = FCIOReadInts(in, FCIOMaxChannels, hwm_cfg->tracemap) / sizeof(*hwm_cfg->tracemap);
  FCIOReadUShorts(in, FCIOMaxChannels, hwm_cfg->fpga_energy_threshold_adc);

  return 0;
}

static inline int fcio_get_fspconfig_ct(FCIOStream in, DSPChannelThreshold* ct_cfg) {
  if (!ct_cfg)
    return -1;
  ct_cfg->ntraces = FCIOReadInts(in, FCIOMaxChannels, ct_cfg->tracemap) / sizeof(*ct_cfg->tracemap);
  FCIOReadUShorts(in, FCIOMaxChannels, ct_cfg->thresholds);

  return 0;
}

static inline int fcio_get_fspconfig_wps(FCIOStream in, DSPWindowedPeakSum* wps_cfg) {
  if (!wps_cfg)
    return -1;
  FCIOReadInt(in, wps_cfg->apply_gain_scaling);
  FCIOReadInt(in, wps_cfg->coincidence_window);
  FCIOReadInt(in, wps_cfg->sum_window_start_sample);
  FCIOReadInt(in, wps_cfg->sum_window_stop_sample);
  FCIOReadFloat(in, wps_cfg->coincidence_threshold);

  wps_cfg->ntraces = FCIOReadInts(in, FCIOMaxChannels, wps_cfg->tracemap) / sizeof(*wps_cfg->tracemap);
  FCIOReadFloats(in, FCIOMaxChannels, wps_cfg->gains);
  FCIOReadFloats(in, FCIOMaxChannels, wps_cfg->thresholds);
  FCIOReadFloats(in, FCIOMaxChannels, wps_cfg->lowpass);
  FCIOReadFloats(in, FCIOMaxChannels, wps_cfg->shaping_widths);

  FCIOReadInt(in, wps_cfg->dsp_pre_max_samples);
  FCIOReadInt(in, wps_cfg->dsp_post_max_samples);
  FCIOReadInts(in, FCIOMaxChannels, wps_cfg->dsp_pre_samples);
  FCIOReadInts(in, FCIOMaxChannels, wps_cfg->dsp_post_samples);
  FCIOReadInts(in, FCIOMaxChannels, wps_cfg->dsp_start_sample);
  FCIOReadInts(in, FCIOMaxChannels, wps_cfg->dsp_stop_sample);

  return 0;
}

static inline int fcio_get_fspconfig(FCIOStream in, StreamProcessor* processor) {
  if (!in || !processor)
    return -1;
  /* StreamProcessor config */
  FCIORead(in, sizeof(processor->config), &processor->config);

  fcio_get_fspconfig_buffer(in, processor->buffer);
  fcio_get_fspconfig_hwm(in, processor->dsp_hwm);
  fcio_get_fspconfig_ct(in, processor->dsp_ct);
  fcio_get_fspconfig_wps(in, processor->dsp_wps);

  return 0;
}


int FCIOGetFSPConfig(FCIOData* input, StreamProcessor* processor)
{
  if (!input || !processor)
    return -1;

  FCIOStream in = FCIOStreamHandle(input);

  return fcio_get_fspconfig(in, processor);
}

static inline int fcio_get_fspevent(FCIOStream in, FSPState* fsp_state) {
  if (!in || !fsp_state)
    return -1;

  FCIORead(in, sizeof(fsp_state->write_flags), &fsp_state->write_flags);
  FCIORead(in, sizeof(fsp_state->proc_flags), &fsp_state->proc_flags);

  FCIORead(in, sizeof(fsp_state->obs.evt), &fsp_state->obs.evt);
  FCIORead(in, sizeof(fsp_state->obs.hwm), &fsp_state->obs.hwm);
  FCIORead(in, sizeof(fsp_state->obs.wps), &fsp_state->obs.wps);

  fsp_state->obs.ct.multiplicity = FCIOReadInts(in, FCIOMaxChannels, fsp_state->obs.ct.trace_idx)/sizeof(int);
  FCIOReadUShorts(in, FCIOMaxChannels, fsp_state->obs.ct.max);

  fsp_state->obs.sub_event_list.size = FCIOReadInts(in, FCIOMaxSamples, fsp_state->obs.sub_event_list.start)/sizeof(int);
  FCIOReadInts(in, FCIOMaxSamples, fsp_state->obs.sub_event_list.stop);
  FCIOReadFloats(in, FCIOMaxSamples, fsp_state->obs.sub_event_list.wps_max);

  return 0;
}

int FCIOGetFSPEvent(FCIOData* input, FSPState* fsp_state)
{
  if (!input || !fsp_state)
    return -1;

  FCIOStream in = FCIOStreamHandle(input);

  return fcio_get_fspevent(in, fsp_state);
}

int FCIOPutFSPEvent(FCIOStream output, FSPState* fsp_state)
{
  if (!output || !fsp_state)
    return -1;

  FCIOWriteMessage(output, FCIOFSPEvent);
  FCIOWrite(output, sizeof(fsp_state->write_flags), &fsp_state->write_flags);
  FCIOWrite(output, sizeof(fsp_state->proc_flags), &fsp_state->proc_flags);

  FCIOWrite(output, sizeof(fsp_state->obs.evt), &fsp_state->obs.evt);
  FCIOWrite(output, sizeof(fsp_state->obs.hwm), &fsp_state->obs.hwm);
  FCIOWrite(output, sizeof(fsp_state->obs.wps), &fsp_state->obs.wps);

  FCIOWriteInts(output, fsp_state->obs.ct.multiplicity, fsp_state->obs.ct.trace_idx);
  FCIOWriteUShorts(output, fsp_state->obs.ct.multiplicity, fsp_state->obs.ct.max);

  FCIOWriteInts(output, fsp_state->obs.sub_event_list.size, fsp_state->obs.sub_event_list.start);
  FCIOWriteInts(output, fsp_state->obs.sub_event_list.size, fsp_state->obs.sub_event_list.stop);
  FCIOWriteFloats(output, fsp_state->obs.sub_event_list.size, fsp_state->obs.sub_event_list.wps_max);

  return FCIOFlush(output);
}

static inline int fcio_get_fspstatus(FCIOStream in, StreamProcessor* processor) {
  if (!in || !processor)
    return -1;

  FCIORead(in, sizeof(FSPStats), processor->stats);

  return 0;
}


int FCIOGetFSPStatus(FCIOData* input, StreamProcessor* processor)
{
  if (!input || !processor)
    return -1;

  FCIOStream in = FCIOStreamHandle(input);

  return fcio_get_fspstatus(in, processor);
}

int FCIOPutFSPStatus(FCIOStream output, StreamProcessor* processor)
{
  if (!output || !processor)
    return -1;

  FCIOWriteMessage(output, FCIOFSPStatus);

  FCIOWrite(output, sizeof(FSPStats), processor->stats);

  return FCIOFlush(output);
}

int FCIOPutFSP(FCIOStream output, StreamProcessor* processor)
{

  if (!output || !processor->buffer->last_fsp_state)
    return -1;

  switch (processor->buffer->last_fsp_state->state->last_tag) {
    case FCIOConfig:
    return FCIOPutFSPConfig(output, processor);

    case FCIOEvent:
    case FCIOSparseEvent:
    case FCIOEventHeader:
    return FCIOPutFSPEvent(output, processor->buffer->last_fsp_state);

    case FCIOStatus:
    return FCIOPutFSPStatus(output, processor);

    default:
    // unknown tag
    return 1;
  }
}

StreamProcessor* FSPCallocStreamProcessor(void)
{
  StreamProcessor* processor = calloc(1, sizeof(StreamProcessor));

  processor->stats = calloc(1, sizeof(FSPStats));
  processor->buffer = calloc(1, sizeof(FSPBuffer));

  processor->dsp_wps = calloc(1, sizeof(DSPWindowedPeakSum));
  processor->dsp_hwm = calloc(1, sizeof(DSPHardwareMajority));
  processor->dsp_ct = calloc(1, sizeof(DSPChannelThreshold));
  return processor;
}

void FSPFreeStreamProcessor(StreamProcessor* processor)
{
  free(processor->dsp_ct);
  free(processor->dsp_hwm);
  free(processor->dsp_wps);
  free(processor->buffer);
  free(processor->stats);

  free(processor);
}
