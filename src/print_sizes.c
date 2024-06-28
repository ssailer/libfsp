#include <stdio.h>
#include <stddef.h>

#include <fsp.h>

void print_size(char* name, size_t size, size_t offset)
{
  const char* units[] = {"b", "B", "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
  int unit_idx = 1;
  while (size > 1024) {
    size /= 1024;
    unit_idx++;
  }

  printf("%s: \toffset %lu B \t size %lu %s\n", name, offset, size, units[unit_idx]);
}


int main(void) {

  FSPWriteFlags write;
  print_size("FSPWriteFlags", sizeof(write), 0);
  print_size(".. EventFlags", sizeof(write.event), offsetof(FSPWriteFlags, event));
  print_size(".. .. write.event.is_retrigger", sizeof(write.event.is_retrigger), offsetof(FSPWriteFlags, event.is_retrigger));
  print_size(".. .. write.event.is_extended", sizeof(write.event.is_extended), offsetof(FSPWriteFlags, event.is_extended));
  print_size(".. STFlags", sizeof(write.trigger), offsetof(FSPWriteFlags, trigger));
  print_size(".. .. write.trigger.hwm_multiplicity", sizeof(write.trigger.hwm_multiplicity), offsetof(FSPWriteFlags, trigger.hwm_multiplicity));
  print_size(".. .. write.trigger.hwm_prescaled", sizeof(write.trigger.hwm_prescaled), offsetof(FSPWriteFlags, trigger.hwm_prescaled));
  print_size(".. .. write.trigger.wps_abs", sizeof(write.trigger.wps_abs), offsetof(FSPWriteFlags, trigger.wps_abs));
  print_size(".. .. write.trigger.wps_rel", sizeof(write.trigger.wps_rel), offsetof(FSPWriteFlags, trigger.wps_rel));
  print_size(".. .. write.trigger.wps_prescaled", sizeof(write.trigger.wps_prescaled), offsetof(FSPWriteFlags, trigger.wps_prescaled));
  print_size(".. .. write.trigger.ct_multiplicity", sizeof(write.trigger.ct_multiplicity), offsetof(FSPWriteFlags, trigger.ct_multiplicity));
  print_size(".. write", sizeof(write.write), offsetof(FSPWriteFlags, write));
  printf("\n");

  FSPProcessorFlags proc;
  print_size("FSPProcessorFlags", sizeof(proc), 0);
  print_size(".. WPSFlags", sizeof(proc.wps), offsetof(FSPProcessorFlags, wps));
  print_size(".. HWMFlags", sizeof(proc.hwm), offsetof(FSPProcessorFlags, hwm));
  print_size(".. CTFlags", sizeof(proc.ct), offsetof(FSPProcessorFlags, ct));
  printf("\n");

  FSPObservables obs;
  print_size("FSPObservables", sizeof(obs), 0);
  print_size(".. wps", sizeof(obs.wps), offsetof(FSPObservables, wps));
  print_size(".. .. wps.max_value", sizeof(obs.wps.max_value), offsetof(FSPObservables, wps.max_value));
  print_size(".. .. wps.max_offset", sizeof(obs.wps.max_offset), offsetof(FSPObservables, wps.max_offset));
  print_size(".. .. wps.max_multiplicity", sizeof(obs.wps.max_multiplicity), offsetof(FSPObservables, wps.max_multiplicity));
  print_size(".. .. wps.max_single_peak_value", sizeof(obs.wps.max_single_peak_value), offsetof(FSPObservables, wps.max_single_peak_value));
  print_size(".. .. wps.max_single_peak_offset", sizeof(obs.wps.max_single_peak_offset), offsetof(FSPObservables, wps.max_single_peak_offset));
  print_size(".. .. sub_event_list", sizeof(obs.sub_event_list), offsetof(FSPObservables, sub_event_list));
  printf("\n");

  print_size(".. hwm", sizeof(obs.hwm), offsetof(FSPObservables, hwm));
  print_size(".. .. hwm.multiplicity", sizeof(obs.hwm.multiplicity), offsetof(FSPObservables, hwm.multiplicity));
  print_size(".. .. hwm.max_value", sizeof(obs.hwm.max_value), offsetof(FSPObservables, hwm.max_value));
  print_size(".. .. hwm.min_value", sizeof(obs.hwm.min_value), offsetof(FSPObservables, hwm.min_value));
  printf("\n");

  print_size(".. ct", sizeof(obs.ct), offsetof(FSPObservables, ct));
  print_size(".. .. ct.multiplicity", sizeof(obs.ct.multiplicity), offsetof(FSPObservables, ct.multiplicity));
  print_size(".. .. ct.max", sizeof(obs.ct.max), offsetof(FSPObservables, ct.max));
  print_size(".. .. ct.trace_idx", sizeof(obs.ct.trace_idx), offsetof(FSPObservables, ct.trace_idx));
  print_size(".. .. ct.label", sizeof(obs.ct.label), offsetof(FSPObservables, ct.label));
  printf("\n");

  print_size(".. event", sizeof(obs.evt), offsetof(FSPObservables, evt));
  print_size(".. .. evt.nextension", sizeof(obs.evt.nextension), offsetof(FSPObservables, evt.nextension));
  printf("\n");

  return 0;
}
