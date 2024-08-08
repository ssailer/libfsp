#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <tmio.h>
#include <fsp.h>

#include "test.h"

typedef struct {
  size_t protocol;
  size_t config;
  size_t event;
  size_t sparseevent;
  size_t eventheader;
  size_t status;
  size_t fspconfig;
  size_t fspevent;
  size_t fspstatus;

} FCIORecordSizes;

size_t written_bytes(FCIOStream stream, int reset)
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

void print_sizes(FCIORecordSizes sizes)
{
  fprintf(stderr, "..protocol    %zu bytes\n", sizes.protocol);
  fprintf(stderr, "..config      %zu bytes\n", sizes.config);
  fprintf(stderr, "..event       %zu bytes\n", sizes.event);
  fprintf(stderr, "..sparseevent %zu bytes\n", sizes.sparseevent);
  fprintf(stderr, "..eventheader %zu bytes\n", sizes.eventheader);
  fprintf(stderr, "..status      %zu bytes\n", sizes.status);
  fprintf(stderr, "..fspconfig   %zu bytes\n", sizes.fspconfig);
  fprintf(stderr, "..fspevent    %zu bytes\n", sizes.fspevent);
  fprintf(stderr, "..fspstatus   %zu bytes\n", sizes.fspstatus);
}

FCIORecordSizes measure_stream_sizes(FCIOData* data, StreamProcessor* processor, FSPState* fspstate)
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


static inline size_t event_size(FCIOTag tag, const fcio_event* event, const fcio_config* config)
{
  const size_t frame_header = sizeof(int);
  size_t total_size = 0;

  total_size += frame_header; // tag_size
  total_size += frame_header + sizeof(((fcio_event){0}).type); // type_size
  total_size += frame_header + sizeof(((fcio_event){0}).pulser); // pulser_size
  total_size += frame_header + sizeof(*((fcio_event){0}).timeoffset) * event->timeoffset_size; // timeoffset_size
  total_size += frame_header + sizeof(*((fcio_event){0}).timestamp) * event->timestamp_size; // timestamp_size
  total_size += frame_header + sizeof(*((fcio_event){0}).deadregion) * event->deadregion_size; // deadregion_size
  switch (tag) {
    case FCIOEvent:
    total_size += frame_header + sizeof(*((fcio_event){0}).traces) * ((config->adcs+config->triggers) * (config->eventsamples+2)); // traces
    break;
    case FCIOSparseEvent:
    total_size += frame_header + sizeof(((fcio_event){0}).num_traces); // num_traces
    total_size += frame_header + sizeof(*((fcio_event){0}).trace_list) * event->num_traces; // trace_list
    total_size += event->num_traces * (frame_header + (config->eventsamples+2) * sizeof(*((fcio_event){0}).traces)); // individual traces
    break;
    case FCIOEventHeader:
    total_size += frame_header + sizeof(*((fcio_event){0}).trace_list) * event->num_traces; // trace_list
    total_size += frame_header + sizeof(unsigned short) * event->num_traces * 2; // headerbuffer
    break;
    default:
      return 0;
  }
  return total_size;
}

static inline size_t config_size(const fcio_config* config)
{
  const size_t frame_header = sizeof(int);
  size_t total_size = 0;

  total_size += frame_header; // tag_size
  total_size += frame_header + sizeof(((fcio_config){0}).adcs);
  total_size += frame_header + sizeof(((fcio_config){0}).triggers);
  total_size += frame_header + sizeof(((fcio_config){0}).eventsamples);
  total_size += frame_header + sizeof(((fcio_config){0}).blprecision);
  total_size += frame_header + sizeof(((fcio_config){0}).sumlength);
  total_size += frame_header + sizeof(((fcio_config){0}).adcbits);
  total_size += frame_header + sizeof(((fcio_config){0}).mastercards);
  total_size += frame_header + sizeof(((fcio_config){0}).triggercards);
  total_size += frame_header + sizeof(((fcio_config){0}).adccards);
  total_size += frame_header + sizeof(((fcio_config){0}).gps);
  total_size += frame_header + sizeof(*((fcio_config){0}).tracemap) * (config->adcs+config->triggers);
  return total_size;
}

static inline size_t status_size(const fcio_status* status)
{
  const size_t frame_header = sizeof(int);
  size_t total_size = 0;

  total_size += frame_header; // tag_size
  total_size += frame_header + sizeof(((fcio_status){0}).status);
  total_size += frame_header + sizeof(*((fcio_status){0}).statustime) * 10;
  total_size += frame_header + sizeof(((fcio_status){0}).cards);
  total_size += frame_header + sizeof(((fcio_status){0}).size);
  total_size += (frame_header + status->size) * status->cards;
  return total_size;
}

static inline size_t fspconfig_size(StreamProcessor* processor)
{
  const size_t frame_header = sizeof(int);
  size_t total_size = 0;

  total_size += frame_header; // tag_size
  total_size += frame_header + sizeof(((FSPConfig){0}));

  total_size += frame_header + sizeof(((FSPBuffer){0}).max_states);
  total_size += frame_header + sizeof(((Timestamp){0}).seconds);
  total_size += frame_header + sizeof(((Timestamp){0}).nanoseconds);

  total_size += frame_header + sizeof(*((DSPHardwareMajority){0}).tracemap) * processor->dsp_hwm->ntraces;
  total_size += frame_header + sizeof(*((DSPHardwareMajority){0}).fpga_energy_threshold_adc) * processor->dsp_hwm->ntraces;

  total_size += frame_header + sizeof(*((DSPChannelThreshold){0}).tracemap) * processor->dsp_ct->ntraces;
  total_size += frame_header + sizeof(*((DSPChannelThreshold){0}).thresholds) * processor->dsp_ct->ntraces;

  total_size += frame_header + sizeof(((DSPWindowedPeakSum){0}).apply_gain_scaling);
  total_size += frame_header + sizeof(((DSPWindowedPeakSum){0}).coincidence_window);
  total_size += frame_header + sizeof(((DSPWindowedPeakSum){0}).sum_window_start_sample);
  total_size += frame_header + sizeof(((DSPWindowedPeakSum){0}).sum_window_stop_sample);
  total_size += frame_header + sizeof(((DSPWindowedPeakSum){0}).coincidence_threshold);

  total_size += frame_header + sizeof(*((DSPWindowedPeakSum){0}).tracemap) * processor->dsp_wps->ntraces;
  total_size += frame_header + sizeof(*((DSPWindowedPeakSum){0}).gains) * processor->dsp_wps->ntraces;
  total_size += frame_header + sizeof(*((DSPWindowedPeakSum){0}).thresholds) * processor->dsp_wps->ntraces;
  total_size += frame_header + sizeof(*((DSPWindowedPeakSum){0}).lowpass) * processor->dsp_wps->ntraces;
  total_size += frame_header + sizeof(*((DSPWindowedPeakSum){0}).shaping_widths) * processor->dsp_wps->ntraces;

  total_size += frame_header + sizeof(((DSPWindowedPeakSum){0}).dsp_pre_max_samples);
  total_size += frame_header + sizeof(((DSPWindowedPeakSum){0}).dsp_post_max_samples);
  total_size += frame_header + sizeof(*((DSPWindowedPeakSum){0}).dsp_pre_samples) * processor->dsp_wps->ntraces;
  total_size += frame_header + sizeof(*((DSPWindowedPeakSum){0}).dsp_post_samples) * processor->dsp_wps->ntraces;
  total_size += frame_header + sizeof(*((DSPWindowedPeakSum){0}).dsp_start_sample) * processor->dsp_wps->ntraces;
  total_size += frame_header + sizeof(*((DSPWindowedPeakSum){0}).dsp_stop_sample) * processor->dsp_wps->ntraces;
  return total_size;
}

static inline size_t fspevent_size(FSPState* fspstate)
{
  const size_t frame_header = sizeof(int);
  size_t total_size = 0;

  total_size += frame_header; // tag_size
  total_size += frame_header + sizeof(FSPWriteFlags);
  total_size += frame_header + sizeof(FSPProcessorFlags);
  total_size += frame_header + sizeof(evt_obs);
  total_size += frame_header + sizeof(hwm_obs);
  total_size += frame_header + sizeof(wps_obs);

  total_size += frame_header + sizeof(*((ct_obs){0}).trace_idx) * fspstate->obs.ct.multiplicity;
  total_size += frame_header + sizeof(*((ct_obs){0}).max) * fspstate->obs.ct.multiplicity;
  total_size += frame_header + sizeof(*((SubEventList){0}).start) * fspstate->obs.sub_event_list.size;
  total_size += frame_header + sizeof(*((SubEventList){0}).stop) * fspstate->obs.sub_event_list.size;
  total_size += frame_header + sizeof(*((SubEventList){0}).wps_max) * fspstate->obs.sub_event_list.size;

  return total_size;
}

static inline size_t fspstatus_size()
{
  const size_t frame_header = sizeof(int);
  size_t total_size = 0;

  total_size += frame_header; // tag_size
  total_size += frame_header + sizeof(FSPStats);

  return total_size;
}

FCIORecordSizes calculate_stream_sizes(FCIOData* data, StreamProcessor* processor, FSPState* fspstate)
{

  FCIORecordSizes sizes = {0};

  sizes.protocol = 68;
  sizes.config = config_size(&data->config);
  sizes.event = event_size(FCIOEvent, &data->event, &data->config);
  sizes.status = status_size(&data->status);
  sizes.eventheader = event_size(FCIOEventHeader, &data->event, &data->config);
  sizes.sparseevent = event_size(FCIOSparseEvent, &data->event, &data->config);
  sizes.fspconfig = fspconfig_size(processor);
  sizes.fspevent = fspevent_size(fspstate);
  sizes.fspstatus = fspstatus_size();

  return sizes;
}

void set_parameters(FCIOData* data, StreamProcessor* processor, FSPState* fspstate, unsigned int nchannels, unsigned int nsamples, int verbose)
{
  processor->dsp_hwm->ntraces = (nchannels) * 3 / 5;
  processor->dsp_wps->ntraces = (nchannels) * 2 / 5;
  processor->dsp_ct->ntraces = nchannels - processor->dsp_hwm->ntraces - processor->dsp_wps->ntraces;

  fspstate->obs.ct.multiplicity = processor->dsp_ct->ntraces;
  fspstate->obs.sub_event_list.size = 2;

  data->config.adcs = processor->dsp_hwm->ntraces + processor->dsp_wps->ntraces + processor->dsp_ct->ntraces;
  data->config.adccards = data->config.adcs / 24 + ((data->config.adcs % 24) == 0 ? 0 : 1);
  data->config.triggercards = data->config.adccards / 8 + ((data->config.adccards % 8) == 0 ? 0 : 1);
  data->config.triggers = data->config.triggercards * 8;
  data->config.mastercards = 1;
  data->config.eventsamples = nsamples;

  data->event.timestamp_size = 10;
  data->event.timeoffset_size = 10;
  data->event.deadregion_size = 10;
  data->event.num_traces = data->config.adcs + data->config.triggers;

  data->status.cards = data->config.adccards + data->config.triggercards + data->config.mastercards;
  data->status.size = sizeof(card_status);
  if (verbose) {
    fprintf(stderr,
      "set_parameters:\n"
      "config: eventsamples %d adcs %d triggers %d\n"
      "config: adccards %d triggercards %d mastercard %d\n"
      "event:  num_traces %d\n"
      "status: cards %d size %d\n"
      "fspevent: ct.multiplicity %d sub_event_list.size %d\n"
      "fspconfig: hwm.ntraces %d wps.ntraces %d ct.ntraces %d\n",
      data->config.eventsamples, data->config.adcs, data->config.triggers, data->config.adccards, data->config.triggercards, data->config.mastercards,
      data->event.num_traces, data->status.cards, data->status.size,
      fspstate->obs.ct.multiplicity, fspstate->obs.sub_event_list.size,
      processor->dsp_hwm->ntraces, processor->dsp_wps->ntraces, processor->dsp_ct->ntraces
    );
  }
}

void check(FCIOData* data, StreamProcessor* processor, FSPState* fspstate, unsigned int nchannels, unsigned int nsamples, int verbose) {

  set_parameters(data, processor, fspstate, nchannels, nsamples, verbose);
  FCIORecordSizes measured_sizes = measure_stream_sizes(data, processor, fspstate);
  FCIORecordSizes calculated_sizes = calculate_stream_sizes(data, processor, fspstate);

  if (verbose) {
    fprintf(stderr, "measured:\n");
    print_sizes(measured_sizes);
    fprintf(stderr, "calculated:\n");
    print_sizes(calculated_sizes);
    fprintf(stderr, "\n");
  }

  assert(measured_sizes.protocol == calculated_sizes.protocol);
  assert(measured_sizes.config == calculated_sizes.config);
  assert(measured_sizes.event == calculated_sizes.event);
  assert(measured_sizes.sparseevent == calculated_sizes.sparseevent);
  assert(measured_sizes.eventheader == calculated_sizes.eventheader);
  assert(measured_sizes.status == calculated_sizes.status);
  assert(measured_sizes.fspconfig == calculated_sizes.fspconfig);
  assert(measured_sizes.fspevent == calculated_sizes.fspevent);
  assert(measured_sizes.fspstatus == calculated_sizes.fspstatus);
}

int main(int argc, char* argv[])
{
  int verbose = 0;
  if (argc <= 2)
    verbose = atoi(argv[1]);
  else
    return 1;

  FCIOData* data = calloc(1, sizeof(FCIOData));
  StreamProcessor* processor = FSPCallocStreamProcessor();
  FSPState* fspstate = calloc(1, sizeof(FSPState));

  check(data, processor, fspstate, 1, 2, verbose); // min

  check(data, processor, fspstate, 1764, 128, verbose); // fc camera default
  check(data, processor, fspstate, 1764, 4096, verbose); // fc camera max
  check(data, processor, fspstate, 2304, 8192, verbose); // max 12-bit

  check(data, processor, fspstate, 181, 8192, verbose); // lgnd
  check(data, processor, fspstate, 181, 6144, verbose); // lgnd better
  check(data, processor, fspstate, 181, 32768, verbose); // lgnd fft
  check(data, processor, fspstate, 576, FCIOMaxSamples, verbose); // max 16-bit


  free(data);
  FSPFreeStreamProcessor(processor);
  free(fspstate);

  return 0;
}
