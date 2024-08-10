#pragma once

#include "state.h"
#include "timestamps.h"

typedef struct FSPBuffer {
  int insert_state;
  int fetch_state;
  int max_states;
  FSPState *fsp_states;

  int nrecords_inserted;
  int nrecords_fetched;
  int fill_level;

  int flush_buffer;

  Timestamp buffer_timestamp;
  Timestamp buffer_window;

  // contains the last state returned via FSPBufferFetchState
  FSPState *last_fsp_state;

} FSPBuffer;

FSPBuffer *FSPBufferCreate(unsigned int buffer_depth, Timestamp buffer_window);
void FSPBufferDestroy(FSPBuffer *buffer);
FSPState *FSPBufferGetState(FSPBuffer *buffer, int offset);
FSPState *FSPBufferPeekState(FSPBuffer *buffer);
void FSPBufferCommitState(FSPBuffer *buffer);
FSPState *FSPBufferFetchState(FSPBuffer *buffer);
int FSPBufferFillLevel(FSPBuffer *buffer);
int FSPBufferFreeLevel(FSPBuffer *buffer);
int FSPBufferFlush(FSPBuffer *buffer);
