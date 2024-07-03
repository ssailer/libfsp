#pragma once

#include "fsp/state.h"
#include "fsp/processor.h"

#include <fcio.h>

int fsp_process(StreamProcessor* processor, FSPState* fsp_state, FCIOState* state);
