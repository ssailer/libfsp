

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "fsp/io_fcio.h"
#include "fcio_utils.h"

#include "test.h"

void print_sizes(FCIORecordSizes sizes)
{
  fprintf(stderr, "..protocol    %zu bytes\n", sizes.protocol);
  fprintf(stderr, "..fspconfig   %zu bytes\n", sizes.fspconfig);
  fprintf(stderr, "..fspevent    %zu bytes\n", sizes.fspevent);
  fprintf(stderr, "..fspstatus   %zu bytes\n", sizes.fspstatus);
}

void set_parameters(StreamProcessor* processor, FSPState* fspstate, unsigned int nchannels, int verbose)
{
  processor->dsp_hwm->ntraces = (nchannels) * 3 / 5;
  processor->dsp_wps->ntraces = (nchannels) * 2 / 5;
  processor->dsp_ct->ntraces = nchannels - processor->dsp_hwm->ntraces - processor->dsp_wps->ntraces;

  fspstate->obs.ct.multiplicity = processor->dsp_ct->ntraces;
  fspstate->obs.sub_event_list.size = 2;

  if (verbose) {
    fprintf(stderr,
      "set_parameters:\n"
      "fspevent: ct.multiplicity %d sub_event_list.size %d\n"
      "fspconfig: hwm.ntraces %d wps.ntraces %d ct.ntraces %d\n",
      fspstate->obs.ct.multiplicity, fspstate->obs.sub_event_list.size,
      processor->dsp_hwm->ntraces, processor->dsp_wps->ntraces, processor->dsp_ct->ntraces
    );
  }
}

void check(StreamProcessor* processor, FSPState* fspstate, unsigned int nchannels, int verbose) {

  set_parameters(processor, fspstate, nchannels, verbose);
  FCIORecordSizes measured_sizes = {0};
  FCIORecordSizes calculated_sizes = {0};
  FSPMeasureRecordSizes(processor, fspstate, &measured_sizes);
  FSPCalculateRecordSizes(processor, fspstate, &calculated_sizes);

  if (verbose) {
    fprintf(stderr, "measured:\n");
    print_sizes(measured_sizes);
    fprintf(stderr, "calculated:\n");
    print_sizes(calculated_sizes);
    fprintf(stderr, "\n");
  }

  assert(measured_sizes.fspconfig == calculated_sizes.fspconfig);
  assert(measured_sizes.fspevent == calculated_sizes.fspevent);
  assert(measured_sizes.fspstatus == calculated_sizes.fspstatus);
}

int main(int argc, char* argv[])
{
  int verbose = 0;
  if (argc >= 2)
    verbose = atoi(argv[1]);

  StreamProcessor* processor = FSPCreate(0);
  FSPState* fspstate = calloc(1, sizeof(FSPState));

  check(processor, fspstate, 0, verbose); // min
  check(processor, fspstate, 1, verbose); // min
  check(processor, fspstate, 181, verbose); // lgnd
  check(processor, fspstate, 576, verbose); // max 16-bit
  check(processor, fspstate, 2304, verbose); // max 12-bit

  FSPDestroy(processor);
  free(fspstate);

  return 0;
}
