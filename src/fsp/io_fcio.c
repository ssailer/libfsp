#include "fsp/io_fcio.h"

#include <fcio.h>
#include <tmio.h>

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


static inline size_t written_bytes(FCIOStream stream, int reset)
{
  static size_t written = 0;
  if (reset) {
    written = 0;
    return 0;
  }
  tmio_stream* tmio = (tmio_stream*)stream;

  size_t new = tmio->byteswritten - written;
  written = tmio->byteswritten;
  return new;
}

FCIORecordSizes FSPMeasureRecordSizes(FCIOData* data, StreamProcessor* processor, FSPState* fspstate)
{
  const char* null_device = "file:///dev/null";

  FCIORecordSizes sizes = {0};

  written_bytes(NULL, 1);
  FCIOStream stream = FCIOConnect(null_device, 'w', 0, 0);
  sizes.protocol = written_bytes(stream, 0);

  size_t current_size = 0;
  int rc = 0;

  rc = FCIOPutConfig(stream, data);
  if ((current_size = written_bytes(stream, 0)) && !rc)
    sizes.config = current_size;

  rc = FCIOPutEvent(stream, data);
  if ((current_size = written_bytes(stream, 0)) && !rc)
    sizes.event = current_size;

  rc = FCIOPutStatus(stream, data);
  if ((current_size = written_bytes(stream, 0)) && !rc)
    sizes.status = current_size;

  rc = FCIOPutEventHeader(stream, data);
  if ((current_size = written_bytes(stream, 0)) && !rc)
    sizes.eventheader = current_size;

  rc = FCIOPutSparseEvent(stream, data);
  if ((current_size = written_bytes(stream, 0)) && !rc)
    sizes.sparseevent = current_size;

  rc = FCIOPutFSPConfig(stream, processor);
  if ((current_size = written_bytes(stream, 0)) && !rc)
    sizes.fspconfig = current_size;

  rc = FCIOPutFSPEvent(stream, fspstate);
  if ((current_size = written_bytes(stream, 0)) && !rc)
    sizes.fspevent = current_size;

  rc = FCIOPutFSPStatus(stream, processor);
  if ((current_size = written_bytes(stream, 0)) && !rc)
    sizes.fspstatus = current_size;

  FCIODisconnect(stream);

  return sizes;
}
