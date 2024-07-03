#pragma once

#include "fsp/timestamps.h"
#include "fsp/flags.h"
#include "fsp/obs.h"

#include <fcio.h>

typedef struct FSPState {
  /* internal */
  FCIOState *state;
  Timestamp timestamp;
  Timestamp unixstamp;
  int has_timestamp;
  int in_buffer;
  int stream_tag;

  /* condense observables into flags */
  FSPWriteFlags write_flags;
  FSPProcessorFlags proc_flags;
  /* calculate observables if event */
  FSPObservables obs;

} FSPState;
