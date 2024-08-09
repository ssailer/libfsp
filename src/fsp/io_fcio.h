#pragma once

#include "fsp/processor.h"
#include <fcio.h>

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

FCIORecordSizes FSPMeasureRecordSizes(FCIOData* data, StreamProcessor* processor, FSPState* fspstate);

int FCIOGetFSPEvent(FCIOData* input, FSPState* fsp_state);
int FCIOGetFSPConfig(FCIOData* input, StreamProcessor* processor);
int FCIOGetFSPStatus(FCIOData* input, StreamProcessor* processor);

int FCIOPutFSPConfig(FCIOStream output, StreamProcessor* processor);
int FCIOPutFSPEvent(FCIOStream output, FSPState* fsp_state);
int FCIOPutFSPStatus(FCIOStream output, StreamProcessor* processor);

int FCIOPutFSP(FCIOStream output, StreamProcessor* processor);
