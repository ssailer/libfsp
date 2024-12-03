#include <stdlib.h>

#include <fsp/io_fcio.h>
#include "fsp/processor.h"

#include "test.h"
#include "fsp_test_utils.h"

#define FCIODEBUG 0
int main(int argc, char* argv[])
{
  assert(argc == 2);

  const char* peer = argv[1];

  FCIODebug(FCIODEBUG);
  int tag = 0;

  /* write test file*/
  FCIOStream stream = FCIOConnect(peer, 'w', 0, 0);
  FCIOData* input = FCIOOpen(peer, 0, 0);

  StreamProcessor* proc_out = FSPCreate(0);
  StreamProcessor* proc_in = FSPCreate(0);

  FSPState* state_out = calloc(1, sizeof(FSPState));
  FSPState* state_in = calloc(1, sizeof(FSPState));

  fill_default_fspconfig(proc_out);
  FCIOPutFSPConfig(stream, proc_out);
  tag = FCIOGetRecord(input);
  assert(tag == FCIOFSPConfig);
  FCIOGetFSPConfig(input,proc_in);
  assert(is_same_fspconfig(proc_in, proc_out));

  fill_default_fspevent(state_out);
  FCIOPutFSPEvent(stream, state_out);
  tag = FCIOGetRecord(input);
  assert(tag == FCIOFSPEvent);
  FCIOGetFSPEvent(input, state_in);
  assert(is_same_fspevent(state_in, state_out));

  fill_default_fspstatus(proc_out);
  FCIOPutFSPStatus(stream, proc_out);
  tag = FCIOGetRecord(input);
  assert(tag == FCIOFSPStatus);
  FCIOGetFSPStatus(input,proc_in);
  assert(is_same_fspstatus(proc_in, proc_out));


  FCIODisconnect(stream);


  FCIOClose(input);

  return 0;

}
