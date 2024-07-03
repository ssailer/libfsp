#pragma once

#include <stddef.h>

#include "fsp/processor.h"
#include <fcio.h>

void FSPFlags2BitString(FSPState* fsp_state, size_t strlen, char* trigger_string, char* event_string);
void FSPBitField2Flags(FSPState* fsp_state, uint32_t trigger_field, uint32_t event_field);
void FSPFlags2BitField(FSPState* fsp_state, uint32_t* trigger_field, uint32_t* event_field);

void FSPFlags2Char(FSPState *fsp_state, size_t strlen, char *cstring);

int FCIOPutFSPConfig(FCIOStream output, StreamProcessor* processor);
int FCIOPutFSPEvent(FCIOStream output, FSPState* fsp_state);
int FCIOPutFSPStatus(FCIOStream output, StreamProcessor* processor);
int FCIOPutFSP(FCIOStream output, StreamProcessor* processor);

void FCIOGetFSPEvent(FCIOData* input, FSPState* fsp_state);
void FCIOGetFSPConfig(FCIOData* input, StreamProcessor* processor);
void FCIOGetFSPStatus(FCIOData* input, StreamProcessor* processor);

void FSPFreeStreamProcessor(StreamProcessor* processor);
StreamProcessor* FSPCallocStreamProcessor();